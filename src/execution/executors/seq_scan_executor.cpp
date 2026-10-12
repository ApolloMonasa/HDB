#include "hdb/execution/executors/seq_scan_executor.h"

#include "hdb/common/exception.h"

namespace hdb {

void SeqScanExecutor::Init() {
    throw NotImplementedException("Lab4: implement SeqScanExecutor::Init (docs/lab4-query.md)");
}

bool SeqScanExecutor::Next(Tuple *tuple, RID *rid) {
    (void)tuple;
    (void)rid;
    throw NotImplementedException("Lab4: implement SeqScanExecutor::Next");
}

}  // namespace hdb
