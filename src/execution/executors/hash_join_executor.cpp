#include "hdb/execution/executors/hash_join_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void HashJoinExecutor::Init() {
    throw NotImplementedException("Lab4: implement HashJoinExecutor::Init (docs/lab4-query.md)");
}

bool HashJoinExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement HashJoinExecutor::Next");
}

}  // namespace hdb
