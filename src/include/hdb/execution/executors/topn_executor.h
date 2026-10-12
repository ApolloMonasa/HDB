#pragma once

#include <vector>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：TopN 执行器。
 *
 * 维护大小为 N 的堆代替全排序：O(rows*logN) 优于 O(rows*logrows)。
 * 堆的序方向与输出方向相反（最大堆出升序前 N 小），想清楚再写。
 */
class TopNExecutor : public AbstractExecutor {
  public:
    TopNExecutor(ExecutorContext *exec_ctx, const TopNPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "TopN"; }

  private:
    std::unique_ptr<AbstractExecutor> child_;
    std::vector<Tuple> heap_rows_;
    size_t out_pos_{0};
};
}  // namespace hdb
