#include "hdb/execution/executors/aggregation_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void AggregationExecutor::Init() {
    throw NotImplementedException("Lab4: implement AggregationExecutor::Init (docs/lab4-query.md)");
}

bool AggregationExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement AggregationExecutor::Next");
}

}  // namespace hdb
