#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：嵌套循环连接执行器。
 *
 * 左行驻外层，右表整表扫内层；谓词以 (left, right) 双元组求值。
 * LEFT JOIN：左行无匹配时输出右槽为 NULL 值的补齐行。
 */
class NestedLoopJoinExecutor : public AbstractExecutor {
  public:
    NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "NLJ"; }

  private:
    std::unique_ptr<AbstractExecutor> left_;
    std::unique_ptr<AbstractExecutor> right_;
    Tuple cur_left_;
    bool left_valid_{false};
    bool left_matched_{false};
};
}  // namespace hdb
