#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hdb/catalog/catalog.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/sql/ast.h"

namespace hdb {

/**
 * @brief 查询计划器（框架给定，含 Binder 职责）。
 *
 * 三件事：
 *   1. **名字绑定**：把 AST 里的表名/列名翻译成 Catalog 句柄和列下标；
 *   2. **SELECT * 展开**：按表 schema 展开成具体列；
 *   3. **计划树组装**：FROM/JOIN → WHERE → GROUP/聚合 → ORDER → LIMIT
 *      自底向上挂节点，并推导每层的输出 schema。
 *
 * CREATE/DROP TABLE 不走计划（shell 直接操作 Catalog）。
 */
class Planner {
  public:
    explicit Planner(Catalog *catalog) : catalog_(catalog) {}

    /** @return DML/SELECT 的计划树 */
    std::unique_ptr<AbstractPlanNode> Plan(const sql::Statement *stmt);

  private:
    std::unique_ptr<AbstractPlanNode> PlanSelect(const sql::SelectStmt *stmt);
    std::unique_ptr<AbstractPlanNode> PlanInsert(const sql::InsertStmt *stmt);
    std::unique_ptr<AbstractPlanNode> PlanDelete(const sql::DeleteStmt *stmt);
    std::unique_ptr<AbstractPlanNode> PlanUpdate(const sql::UpdateStmt *stmt);

    /**
     * AST 表达式 → 可执行表达式。
     * @param scopes  当前可见的 (别名/表名, schema) 序列；join 后左右表都在列
     * @param default_idx 非限定列名缺省落在哪个槽（单表 = 0）
     * @param single_stream 非空时进入"单流模式"：列下标按该 schema 解析且槽位恒为
     *                      左槽（join 之上的 Sort 等算子只见一条合并元组流时使用）
     */
    AbstractExpressionRef BindExpr(const sql::Expr *expr,
                                   const std::vector<std::pair<std::string, Schema>> &scopes,
                                   int default_idx, const Schema *single_stream = nullptr);

    Catalog *catalog_;
};

}  // namespace hdb
