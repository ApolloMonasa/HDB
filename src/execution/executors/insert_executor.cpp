#include "hdb/execution/executors/insert_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void InsertExecutor::Init() {
    throw NotImplementedException("Lab4: implement InsertExecutor::Init (docs/lab4-query.md)");
}

bool InsertExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement InsertExecutor::Next");
}

}  // namespace hdb
