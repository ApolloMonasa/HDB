#include "hdb/execution/executors/limit_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void LimitExecutor::Init() {
    throw NotImplementedException("Lab4: implement LimitExecutor::Init (docs/lab4-query.md)");
}

bool LimitExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement LimitExecutor::Next");
}

}  // namespace hdb
