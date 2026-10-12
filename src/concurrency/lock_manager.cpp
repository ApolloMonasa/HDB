#include "hdb/concurrency/lock_manager.h"

#include "hdb/common/exception.h"

namespace hdb {

// 实现说明：Lab5 的填空位。锁表结构与请求队列已给定，
// 你要写的是"谁该拿锁、谁该等、谁被唤醒"的调度逻辑。
// 教材对照：《数据库系统概论》封锁协议一节的算法描述可以直接翻译成代码。

void LockManager::LockShared(Transaction *txn, const std::string &table) {
    (void)txn;
    (void)table;
    throw NotImplementedException("Lab5: implement LockManager::LockShared (docs/lab5-txn.md)");
}

void LockManager::LockExclusive(Transaction *txn, const std::string &table) {
    (void)txn;
    (void)table;
    throw NotImplementedException("Lab5: implement LockManager::LockExclusive");
}

void LockManager::LockUpgrade(Transaction *txn, const std::string &table) {
    (void)txn;
    (void)table;
    throw NotImplementedException("Lab5: implement LockManager::LockUpgrade");
}

void LockManager::Unlock(Transaction *txn, const std::string &table) {
    (void)txn;
    (void)table;
    throw NotImplementedException("Lab5: implement LockManager::Unlock");
}

size_t LockManager::GrantedCount(const std::string &table) {
    std::lock_guard<std::mutex> lk(table_lock_);
    auto it = lock_table_.find(table);
    if (it == lock_table_.end()) {
        return 0;
    }
    size_t n = 0;
    for (const auto &r : it->second.requests_) {
        if (r.granted_) {
            n++;
        }
    }
    return n;
}

}  // namespace hdb
