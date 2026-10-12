#pragma once

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/storage/index/index_iterator.h"

namespace hdb {

/**
 * @brief Lab 4：索引扫描执行器。
 *
 * 走主键 B+ 树：谓词形如 pk = 常量 -> ScanKey 定位；
 * 范围谓词 -> IndexIterator 沿叶子链扫描。
 * 这是优化规则 R1 的受益者。
 */
class IndexScanExecutor : public AbstractExecutor {
  public:
    IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan, const TableInfo &table)
        : AbstractExecutor(exec_ctx, plan), table_(table) {}

    void Init() override;
    bool Next(Tuple *tuple, RID *rid) override;
    const char *Name() const override { return "IndexScan"; }

  private:
    TableInfo table_;
    IndexIterator iter_{};
    bool point_lookup_{false};
    RID point_rid_{};
};
}  // namespace hdb
