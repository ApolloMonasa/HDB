#pragma once

#include <memory>
#include <vector>

#include "hdb/catalog/catalog.h"
#include "hdb/common/rid.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

class Transaction;

class LockManager;

/** 执行器的运行环境：catalog + 锁管理器 + 当前事务（Lab5 起事务参与执行） */
class ExecutorContext {
  public:
    ExecutorContext(Catalog *catalog, Transaction *txn = nullptr, LockManager *lock_mgr = nullptr)
        : catalog_(catalog), txn_(txn), lock_mgr_(lock_mgr) {}

    Catalog *GetCatalog() { return catalog_; }
    Transaction *GetTransaction() { return txn_; }

    /** Lab5 Task3：扫描算子加 S 锁 / 写算子加 X 锁经此入口（未开事务时为空） */
    LockManager *GetLockManager() { return lock_mgr_; }

  private:
    Catalog *catalog_;
    Transaction *txn_;
    LockManager *lock_mgr_;
};

/**
 * @file abstract_executor.h
 * @brief 火山模型执行器基类（框架给定）。
 *
 * 每个算子只有两个动作——这就是火山模型的全部：
 *
 *   Init()          打开算子（构造哈希表、重置游标等一次性动作）
 *   Next(t, rid)    拉取一行；没有更多数据返回 false
 *
 * 执行引擎自顶向下拉取，数据自底向上流动——
 * "面向同学与面向谓词"都是这个循环：while (child->Next(&t)) { if (filter(t)) ... }
 */
class AbstractExecutor {
  public:
    AbstractExecutor(ExecutorContext *exec_ctx, const AbstractPlanNode *plan)
        : exec_ctx_(exec_ctx), plan_(plan) {}

    virtual ~AbstractExecutor() = default;

    virtual void Init() = 0;

    /** @return true 时 *tuple 为新产出的一行 */
    virtual bool Next(Tuple *tuple, RID *rid) = 0;

    /** 本算子输出的行模式 */
    const Schema &OutputSchema() const { return plan_->OutputSchema(); }

    /** 展示用：算子类型名（EXPLAIN / 调试） */
    virtual const char *Name() const { return "AbstractExecutor"; }

  protected:
    ExecutorContext *exec_ctx_;
    const AbstractPlanNode *plan_;
};

}  // namespace hdb
