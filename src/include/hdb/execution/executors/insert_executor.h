#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：插入执行器。
 *
 * 把计划里的字面量行包装成 Tuple 经 TableHeap::InsertTuple 落地；
 * 表有主键索引时同步 InsertEntry（重复主键 -> 报错并停止本批）。
 */
class InsertExecutor : public AbstractExecutor {
  public:
    InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan), table_(plan->GetTable()) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Insert"; }

  private:
    TableInfo table_;
    bool inserted_{false};
};
}  // namespace hdb
