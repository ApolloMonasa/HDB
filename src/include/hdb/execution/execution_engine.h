#pragma once

#include <vector>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

/**
 * @brief 执行引擎（框架给定）：驱动计划树的 Volcano 循环。
 *
 *   for plan in optimized_plans:
 *       executor = ExecutorFactory(plan)
 *       executor->Init()
 *       while (executor->Next(&t, &rid)) result_set.push_back(t)
 */
class ExecutionEngine {
  public:
    /**
     * 执行一条计划。
     * @param[out] result_set SELECT 的结果行（DML 为空）
     * @return 影响行数（INSERT/DELETE/UPDATE），SELECT 返回 0
     */
    int Execute(const AbstractPlanNode *plan, ExecutorContext *exec_ctx,
                std::vector<Tuple> *result_set);
};

}  // namespace hdb
