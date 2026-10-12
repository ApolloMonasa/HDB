#include "hdb/execution/execution_engine.h"

#include "hdb/execution/executor_factory.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

// 给定实现：火山循环的最外层——自顶向下拉取直到 Next 返回 false。
int ExecutionEngine::Execute(const AbstractPlanNode *plan, ExecutorContext *exec_ctx,
                             std::vector<Tuple> *result_set) {
    auto executor = ExecutorFactory::CreateExecutor(exec_ctx, plan);
    executor->Init();

    Tuple tuple;
    RID rid;
    int affected = 0;
    while (executor->Next(&tuple, &rid)) {
        if (result_set != nullptr) {
            result_set->push_back(tuple);
        }
        affected++;
    }
    return affected;
}

}  // namespace hdb
