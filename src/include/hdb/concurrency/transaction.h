#pragma once

#include <atomic>
#include <string>
#include <unordered_set>

#include "hdb/common/config.h"

namespace hdb {

/**
 * @file transaction.h
 * @brief 事务对象（框架给定）——一个事务的全部运行时状态。
 *
 * 2PL 的核心状态机就藏在 state_ 里：
 *
 *   GROWING（增长期）──Commit──► COMMITTED
 *        │                        ▲
 *        │                     Abort
 *        ▼                        │
 *   SHRINKING（收缩期）───────────┘
 *
 * 规则（教材"两阶段封锁协议"）：
 *   * 增长期只能加锁，收缩期开始后（第一次 Unlock）不得再加锁；
 *   * 严格 2PL（S2PL）：所有 X 锁持有到 Commit/Abort 才释放——
 *     HDB 的 LockManager::Unlock 即按 S2PL 语义判定违规。
 */
enum class TxnState { GROWING = 0, SHRINKING, COMMITTED, ABORTED };

class Transaction {
  public:
    explicit Transaction(txn_id_t txn_id, TxnState state = TxnState::GROWING)
        : txn_id_(txn_id), state_(state) {}

    txn_id_t GetTxnId() const { return txn_id_; }
    TxnState GetState() const { return state_; }
    void SetState(TxnState state) { state_ = state; }

    /** 本事务持有的 S/X 锁集合（LockManager 登记用） */
    void AddSLock(const std::string &table) { s_lock_set_.insert(table); }
    void DropSLock(const std::string &table) { s_lock_set_.erase(table); }
    void AddXLock(const std::string &table) { x_lock_set_.insert(table); }
    void DropXLock(const std::string &table) { x_lock_set_.erase(table); }

    const std::unordered_set<std::string> &GetSharedLockSet() const { return s_lock_set_; }
    const std::unordered_set<std::string> &GetExclusiveLockSet() const { return x_lock_set_; }

    bool IsGrowing() const { return state_ == TxnState::GROWING; }
    bool IsShrinking() const { return state_ == TxnState::SHRINKING; }

  private:
    txn_id_t txn_id_;
    TxnState state_;
    std::unordered_set<std::string> s_lock_set_;
    std::unordered_set<std::string> x_lock_set_;
};

}  // namespace hdb
