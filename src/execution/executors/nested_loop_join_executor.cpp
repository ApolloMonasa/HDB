#include "hdb/execution/executors/nested_loop_join_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void NestedLoopJoinExecutor::Init() {
    throw NotImplementedException("Lab4: implement NestedLoopJoinExecutor::Init (docs/lab4-query.md)");
}

bool NestedLoopJoinExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement NestedLoopJoinExecutor::Next");
}

}  // namespace hdb
