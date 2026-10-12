#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/catalog/catalog.h"
#include "hdb/concurrency/transaction_manager.h"
#include "hdb/execution/execution_engine.h"
#include "hdb/execution/planner.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

/** 一条 SQL 的执行结果（shell 打印与 sqllogictest 比对共用） */
struct SqlResult {
    bool ok{false};
    std::string error;
    int affected{0};             // DML 影响行数
    bool has_rows{false};        // SELECT 为 true
    Schema row_schema;           // 行模式（打印表头）
    std::vector<Tuple> rows;     // SELECT 的结果行

    /** 结果渲染为文本（值以 | 分隔，sqllogictest 风格） */
    std::vector<std::string> RenderRows() const;

    /** EXPLAIN 的计划树文本（is_explain 时有效） */
    std::string explain_plan;
};

/**
 * @brief HDB 实例（框架给定）：把全部组件串成一条流水线。
 *
 *   SQL 文本
 *     -> Lexer -> Parser(AST)
 *     -> CREATE/DROP 直接操作 Catalog
 *     -> Planner(计划树) -> Optimizer(改写) -> ExecutionEngine(火山循环)
 */
class HdbInstance {
  public:
    explicit HdbInstance(const std::string &db_file);
    ~HdbInstance();

    HdbInstance(const HdbInstance &) = delete;
    HdbInstance &operator=(const HdbInstance &) = delete;

    /** 执行一条 SQL（不含多语句） */
    SqlResult ExecuteSql(const std::string &sql);

    Catalog *GetCatalog() { return catalog_.get(); }
    BufferPoolManager *GetBufferPoolManager() { return bpm_.get(); }

    /**
     * 事务模式开关（Lab5 Task3）：开启后每条 DML/SELECT 包在一个事务里——
     * 扫描算子加 S 锁、写算子加 X 锁，Commit/Abort 时统一释放（S2PL）。
     * 默认关闭：未完成 Lab5 前执行器不碰锁管理器。
     */
    void SetTxnEnabled(bool enabled) { txn_enabled_ = enabled; }
    bool IsTxnEnabled() const { return txn_enabled_; }
    TransactionManager &GetTransactionManager() { return txn_manager_; }

  private:
    SqlResult HandleCreate(const sql::CreateStmt *stmt);
    SqlResult HandleDrop(const sql::DropStmt *stmt);

    std::unique_ptr<DiskManager> dm_;
    std::unique_ptr<BufferPoolManager> bpm_;
    std::unique_ptr<Catalog> catalog_;
    TransactionManager txn_manager_;
    bool txn_enabled_{false};
    Planner planner_;
    ExecutionEngine engine_;
};

}  // namespace hdb
