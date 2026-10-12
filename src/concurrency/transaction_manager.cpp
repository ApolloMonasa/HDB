#include "hdb/concurrency/transaction_manager.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab5 的填空位。

Transaction *TransactionManager::Begin() {
    throw NotImplementedException("Lab5: implement TransactionManager::Begin (docs/lab5-txn.md)");
}

void TransactionManager::Commit(Transaction *txn) {
    (void)txn;
    throw NotImplementedException("Lab5: implement TransactionManager::Commit");
}

void TransactionManager::Abort(Transaction *txn) {
    (void)txn;
    throw NotImplementedException("Lab5: implement TransactionManager::Abort");
}

}  // namespace hdb
