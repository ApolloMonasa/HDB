#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "hdb/common/config.h"
#include "hdb/concurrency/lock_manager.h"
#include "hdb/concurrency/transaction.h"

namespace hdb {

/**
 * @file transaction_manager.h
 * @brief Lab 5：事务管理器——事务的一生由它经手。
 *
 *   Begin ──► GROWING ──Commit──► 释放全部锁 ──► COMMITTED
 *                │
 *              Abort
 *                ▼
 *          释放全部锁 ──► ABORTED
 *
 * HDB 骨架的事务集合是单连接内的（教学定位），
 * Begin/Commit/Abort 的并发安全靠一把互斥锁即可。
 */
class TransactionManager {
  public:
    TransactionManager() = default;

    /** TODO(Lab5): 新建事务（id 全局单调递增），登记进事务表并返回 */
    Transaction *Begin();

    /**
     * TODO(Lab5): 提交——置 COMMITTED，释放该事务在全部表上的锁
     * （遍历 txn 的 S/X 锁集合调 LockManager::Unlock；S2PL 的释放点）。
     */
    void Commit(Transaction *txn);

    /** TODO(Lab5): 回滚——置 ABORTED，同样释放全部锁。 */
    void Abort(Transaction *txn);

    /** 全局锁管理器（执行器经它对表加锁） */
    LockManager &GetLockManager() { return lock_manager_; }

  private:
    LockManager lock_manager_;
    std::atomic<txn_id_t> next_txn_id_{1};
    std::mutex txn_map_lock_;
    std::vector<std::unique_ptr<Transaction>> txn_table_;  // 持有所有权
};

}  // namespace hdb
