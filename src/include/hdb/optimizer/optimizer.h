#pragma once

#include <memory>

#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @file optimizer.h
 * @brief 基于规则的优化器（框架给定驱动，2 条规则是你的填空）。
 *
 * 与工业级优化器（代价模型+Cascades）相比，规则式优化是"看得见摸得着"
 * 的入门形态：自底向上遍历计划树，命中模式就改写。
 *
 * 你要实现的两条规则（docs/lab4-query.md）：
 *   R1  SeqScan→IndexScan：谓词形如 "pk = 常量" 且表有主键索引时改写
 *   R2  NLJ→HashJoin：连接谓词是等值比较（左列 = 右列）时改写
 *
 * 框架给定的第三条规则：Sort+Limit → TopN（维护大小为 N 的堆代替全排序）。
 */
class Optimizer {
  public:
    /** 驱动：自底向上遍历并逐规则改写（给定） */
    static std::unique_ptr<AbstractPlanNode> Optimize(std::unique_ptr<AbstractPlanNode> plan);

    /** TODO(Lab4-R1)：SeqScan + 主键等值谓词 → IndexScan */
    static std::unique_ptr<AbstractPlanNode> OptimizeSeqScanAsIndexScan(
        std::unique_ptr<AbstractPlanNode> plan);

    /** TODO(Lab4-R2)：NLJ + 等值谓词 → HashJoin */
    static std::unique_ptr<AbstractPlanNode> OptimizeNLJAsHashJoin(
        std::unique_ptr<AbstractPlanNode> plan);

  private:
    /** 给定：Limit(Sort(x)) → TopN(x) */
    static std::unique_ptr<AbstractPlanNode> OptimizeSortLimitAsTopN(
        std::unique_ptr<AbstractPlanNode> plan);
};

}  // namespace hdb
