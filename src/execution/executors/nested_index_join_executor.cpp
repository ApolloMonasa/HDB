#include "hdb/execution/executors/nested_index_join_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void NestedIndexJoinExecutor::Init() {
    throw NotImplementedException("Lab4: implement NestedIndexJoinExecutor::Init (docs/lab4-query.md)");
}

bool NestedIndexJoinExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement NestedIndexJoinExecutor::Next");
}

}  // namespace hdb
