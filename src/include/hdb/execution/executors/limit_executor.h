#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/** @brief Lab 4：LIMIT 执行器——放行前 N 行后立即让下层提前退出。 */
class LimitExecutor : public AbstractExecutor {
  public:
    LimitExecutor(ExecutorContext *exec_ctx, const LimitPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Limit"; }

  private:
    std::unique_ptr<AbstractExecutor> child_;
    int64_t emitted_{0};
};
}  // namespace hdb
