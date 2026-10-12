#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：删除执行器。
 *
 * 拉取 child（通常是带谓词的 SeqScan）产出的每一行，
 * 对其 RID 做 MarkDelete + 主键索引 DeleteEntry。
 */
class DeleteExecutor : public AbstractExecutor {
  public:
    DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan)
        : AbstractExecutor(exec_ctx, plan), table_(plan->GetTable()) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Delete"; }

  private:
    TableInfo table_;
    std::unique_ptr<AbstractExecutor> child_;
};
}  // namespace hdb
