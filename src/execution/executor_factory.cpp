#include "hdb/execution/executor_factory.h"

#include "hdb/execution/executors/aggregation_executor.h"
#include "hdb/execution/executors/delete_executor.h"
#include "hdb/execution/executors/hash_join_executor.h"
#include "hdb/execution/executors/index_scan_executor.h"
#include "hdb/execution/executors/insert_executor.h"
#include "hdb/execution/executors/limit_executor.h"
#include "hdb/execution/executors/nested_index_join_executor.h"
#include "hdb/execution/executors/nested_loop_join_executor.h"
#include "hdb/execution/executors/seq_scan_executor.h"
#include "hdb/execution/executors/sort_executor.h"
#include "hdb/execution/executors/topn_executor.h"
#include "hdb/execution/executors/update_executor.h"

namespace hdb {

// 给定实现：计划类型 -> 执行器实例。
// 学生完成各执行器后无需改动本函数（除非新增算子）。
std::unique_ptr<AbstractExecutor> ExecutorFactory::CreateExecutor(ExecutorContext *exec_ctx,
                                                                  const AbstractPlanNode *plan) {
    switch (plan->GetType()) {
        case PlanType::SEQ_SCAN: {
            const auto *p = static_cast<const SeqScanPlanNode *>(plan);
            return std::make_unique<SeqScanExecutor>(exec_ctx, p, p->GetTable());
        }
        case PlanType::INSERT: {
            return std::make_unique<InsertExecutor>(exec_ctx, static_cast<const InsertPlanNode *>(plan));
        }
        case PlanType::DELETE_: {
            return std::make_unique<DeleteExecutor>(exec_ctx, static_cast<const DeletePlanNode *>(plan));
        }
        case PlanType::UPDATE: {
            return std::make_unique<UpdateExecutor>(exec_ctx, static_cast<const UpdatePlanNode *>(plan));
        }
        case PlanType::INDEX_SCAN: {
            const auto *p = static_cast<const IndexScanPlanNode *>(plan);
            return std::make_unique<IndexScanExecutor>(exec_ctx, p, p->GetTable());
        }
        case PlanType::NESTED_LOOP_JOIN: {
            return std::make_unique<NestedLoopJoinExecutor>(
                exec_ctx, static_cast<const NestedLoopJoinPlanNode *>(plan));
        }
        case PlanType::HASH_JOIN: {
            return std::make_unique<HashJoinExecutor>(exec_ctx, static_cast<const HashJoinPlanNode *>(plan));
        }
        case PlanType::AGGREGATION: {
            return std::make_unique<AggregationExecutor>(exec_ctx,
                                                          static_cast<const AggregationPlanNode *>(plan));
        }
        case PlanType::SORT: {
            return std::make_unique<SortExecutor>(exec_ctx, static_cast<const SortPlanNode *>(plan));
        }
        case PlanType::LIMIT: {
            return std::make_unique<LimitExecutor>(exec_ctx, static_cast<const LimitPlanNode *>(plan));
        }
        case PlanType::TOPN: {
            return std::make_unique<TopNExecutor>(exec_ctx, static_cast<const TopNPlanNode *>(plan));
        }
    }
    throw ExecutionException("executor factory: unknown plan type");
}

}  // namespace hdb
