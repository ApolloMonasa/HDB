#include "hdb/execution/executors/sort_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void SortExecutor::Init() {
    throw NotImplementedException("Lab4: implement SortExecutor::Init (docs/lab4-query.md)");
}

bool SortExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement SortExecutor::Next");
}

}  // namespace hdb
