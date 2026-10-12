#pragma once

#include <utility>
#include <vector>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/executors/agg_hash_table.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：聚合执行器。
 *
 * 借助 SimpleAggregationHashTable 按 GROUP BY 键分桶累计
 * COUNT/SUM/MIN/MAX/AVG；HAVING 过滤桶；DISTINCT 记哈希集合。
 * 注意：无 GROUP BY 的聚合整个输入是一个桶（即使输入为空也要产出一行）。
 */
class AggregationExecutor : public AbstractExecutor {
  public:
    AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Aggregate"; }

  private:
    std::unique_ptr<AbstractExecutor> child_;
    std::unique_ptr<SimpleAggregationHashTable> aht_;
    std::vector<std::pair<AggregateKey, AggregateValue>> results_;
    size_t out_pos_{0};
};
}  // namespace hdb
