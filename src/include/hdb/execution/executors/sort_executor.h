#pragma once

#include <vector>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief Lab 4：排序执行器。
 *
 * 基础要求：全量 materialize 后 std::stable_sort（多键、混合升降序）。
 * 选做：二路外部归并排序（页面当缓冲，练习"数据 > 内存"的取舍）。
 */
class SortExecutor : public AbstractExecutor {
  public:
    SortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan)
        : AbstractExecutor(exec_ctx, plan) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "Sort"; }

  private:
    std::unique_ptr<AbstractExecutor> child_;
    std::vector<Tuple> rows_;
    size_t out_pos_{0};
};
}  // namespace hdb
