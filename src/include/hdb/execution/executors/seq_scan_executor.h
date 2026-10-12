#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/storage/table/table_iterator.h"

namespace hdb {

/**
 * @brief Lab 4：顺序扫描执行器（融合谓词过滤与投影）。
 *
 * 拉取路径：TableHeap 迭代器逐行 -> filter 谓词求值 ->
 * 按本算子 OutputSchema 投影出新元组。
 * 墓碑行天然被 TableIterator 跳过（Lab2 的成果在这里兑现）。
 */
class SeqScanExecutor : public AbstractExecutor {
  public:
    SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan, const TableInfo &table)
        : AbstractExecutor(exec_ctx, plan), table_(table) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "SeqScan"; }

  private:
    TableInfo table_;
    std::unique_ptr<TableIterator> iterator_;
    std::unique_ptr<TableIterator> end_;
};
}  // namespace hdb
