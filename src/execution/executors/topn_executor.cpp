#include "hdb/execution/executors/topn_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void TopNExecutor::Init() {
    throw NotImplementedException("Lab4: implement TopNExecutor::Init (docs/lab4-query.md)");
}

bool TopNExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement TopNExecutor::Next");
}

}  // namespace hdb
