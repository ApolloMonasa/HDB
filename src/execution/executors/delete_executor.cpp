#include "hdb/execution/executors/delete_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void DeleteExecutor::Init() {
    throw NotImplementedException("Lab4: implement DeleteExecutor::Init (docs/lab4-query.md)");
}

bool DeleteExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement DeleteExecutor::Next");
}

}  // namespace hdb
