#pragma once

#include <memory>

#include "hdb/execution/executors/abstract_executor.h"
#include "hdb/execution/plans/plan_node.h"

namespace hdb {

/**
 * @brief 执行器工厂（框架给定）：计划节点 → 执行器实例。
 *
 * 为什么不直接在计划节点上写 Next()？
 * 把"查询的描述"（plan，纯数据）与"查询的执行"（executor，有状态游标）
 * 分开，同一个计划可以被多次执行、被优化器无副作用地改写。
 */
class ExecutorFactory {
  public:
    static std::unique_ptr<AbstractExecutor> CreateExecutor(ExecutorContext *exec_ctx,
                                                            const AbstractPlanNode *plan);
};

}  // namespace hdb
