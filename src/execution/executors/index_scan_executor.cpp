#include "hdb/execution/executors/index_scan_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void IndexScanExecutor::Init() {
    throw NotImplementedException("Lab4: implement IndexScanExecutor::Init (docs/lab4-query.md)");
}

bool IndexScanExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement IndexScanExecutor::Next");
}

}  // namespace hdb
