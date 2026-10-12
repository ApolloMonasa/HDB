#include "hdb/optimizer/optimizer.h"

#include "hdb/common/exception.h"

namespace hdb {

std::unique_ptr<AbstractPlanNode> Optimizer::Optimize(std::unique_ptr<AbstractPlanNode> plan) {
    // 给定驱动：自底向上（先递归优化子树），再对当前节点过三条规则
    for (size_t pass = 0; pass < 3; pass++) {
        plan = OptimizeSeqScanAsIndexScan(std::move(plan));
        plan = OptimizeNLJAsHashJoin(std::move(plan));
        plan = OptimizeSortLimitAsTopN(std::move(plan));
    }
    return plan;
}

std::unique_ptr<AbstractPlanNode> Optimizer::OptimizeSeqScanAsIndexScan(
    std::unique_ptr<AbstractPlanNode> plan) {
    (void)plan;
    throw NotImplementedException("Lab4-R1: implement SeqScan->IndexScan rule (docs/lab4-query.md)");
}

std::unique_ptr<AbstractPlanNode> Optimizer::OptimizeNLJAsHashJoin(
    std::unique_ptr<AbstractPlanNode> plan) {
    (void)plan;
    throw NotImplementedException("Lab4-R2: implement NLJ->HashJoin rule");
}

std::unique_ptr<AbstractPlanNode> Optimizer::OptimizeSortLimitAsTopN(
    std::unique_ptr<AbstractPlanNode> plan) {
    // 给定实现：Limit(Sort(child, obs), n) → TopN(child, obs, n)
    if (plan == nullptr || plan->GetType() != PlanType::LIMIT) {
        return plan;
    }
    auto *limit = static_cast<LimitPlanNode *>(plan.get());
    if (limit->GetChild() == nullptr ||
        limit->GetChild()->GetType() != PlanType::SORT) {
        return plan;
    }
    auto *sort = const_cast<SortPlanNode *>(static_cast<const SortPlanNode *>(limit->GetChild()));

    // 重建：TopN(sort 的 child, sort 的 order_bys, limit 的 n)
    auto child = std::unique_ptr<AbstractPlanNode>(const_cast<AbstractPlanNode *>(sort->GetChild()));
    return std::make_unique<TopNPlanNode>(std::move(child), sort->GetOrderBys(), limit->GetLimit());
}

}  // namespace hdb
