#include "hdb/execution/executors/update_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void UpdateExecutor::Init() {
    throw NotImplementedException("Lab4: implement UpdateExecutor::Init (docs/lab4-query.md)");
}

bool UpdateExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement UpdateExecutor::Next");
}

}  // namespace hdb
