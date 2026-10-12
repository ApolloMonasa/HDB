#include <unordered_set>
#include <vector>

#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：更新执行器。
 *
 * 语义 = 先删后插：对 child 的每行按 sets 重算各列得到新元组，
 * MarkDelete 旧行 + InsertTuple 新行；主键列不可出现在 SET 里。
 */
class UpdateExecutor : public AbstractExecutor {
  public:
    UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan)
        : AbstractExecutor(exec_ctx, plan), table_(plan->GetTable()) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Update"; }

  private:
    TableInfo table_;
    std::unique_ptr<AbstractExecutor> child_;
    std::unordered_set<std::string> processed_pks_;  // 已处理主键（先删后插 rid 会变，主键不变）
};
}  // namespace hdb
