#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/storage/index/index_iterator.h"

namespace hdb {

/**
 * @brief Lab 4：索引嵌套循环连接（选做）。
 *
 * 与 NLJ 同计划类型，但右表存在可命中的索引时，内层循环
 * 走 B+ 树点查代替全表扫描——体会"同一语义，不同代价"。
 */
class NestedIndexJoinExecutor : public AbstractExecutor {
  public:
    NestedIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "IndexNLJ"; }

  private:
    std::unique_ptr<AbstractExecutor> left_;
    IndexIterator iter_{};
    bool inner_active_{false};
};
}  // namespace hdb
