#pragma once

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "hdb/common/config.h"
#include "hdb/concurrency/transaction.h"

namespace hdb {

/** 锁模式：S 共享（多读并存）/ X 排他（独占写） */
enum class LockMode : uint8_t { SHARED = 0, EXCLUSIVE };

/** 一条加锁请求（框架给定） */
struct LockRequest {
    txn_id_t txn_id_;
    LockMode lock_mode_;
    bool granted_{false};

    LockRequest(txn_id_t id, LockMode mode) : txn_id_(id), lock_mode_(mode) {}
};

/** 每张表一个等待队列（框架给定） */
struct LockRequestQueue {
    /** 正在排队/持锁的请求；队首若干 granted_ == true 为当前持锁者 */
    std::list<LockRequest> requests_;
    std::condition_variable cv_;  // 队列状态变化时唤醒等待者
    /** 是否有事务正在升级（升级优先于一切新请求，防饿死） */
    bool upgrading_{false};
};

/**
 * @file lock_manager.h
 * @brief Lab 5：2PL 表级锁管理器。
 *
 * 授锁规则（贴教材封锁协议）：
 *   * S 与 S 兼容（多个读者并存）；S 与 X、X 与 X 互斥；
 *   * 同一队列 FIFO 授锁：新请求必须等队首（即便与已持锁者兼容，
 *     也要排在先到的请求之后——防止写者饿死）；
 *   * 升级（S→X）优先于新请求；两个事务同时升级 = 死锁，
 *     抛 LockAbortException 让其中一方回滚重来；
 *   * Unlock 时若事务仍处 GROWING → 进入 SHRINKING；
 *     SHRINKING 后再请求锁 → LockAbortException（2PL 违规）。
 *
 * 等待实现方式建议：条件变量 + 谓词（"队首是我且与前面持锁者兼容"）。
 */
class LockManager {
  public:
    LockManager() = default;

    /** TODO(Lab5): 申请共享锁。队列不兼容则阻塞等待。 */
    void LockShared(Transaction *txn, const std::string &table);

    /** TODO(Lab5): 申请排他锁。 */
    void LockExclusive(Transaction *txn, const std::string &table);

    /**
     * TODO(Lab5): S -> X 升级（事务已持有该表的 S 锁）。
     * 升级期间其他事务的任何请求都应等待；并发升级抛 LockAbortException。
     */
    void LockUpgrade(Transaction *txn, const std::string &table);

    /**
     * TODO(Lab5): 释放该事务在此表上的锁并唤醒队列。
     * GROWING 期首次 Unlock 把事务置为 SHRINKING；
     * 未持锁却 Unlock 属于 bug，assert 掉。
     */
    void Unlock(Transaction *txn, const std::string &table);

    /** 测试辅助：某表当前 granted 的请求数 */
    size_t GrantedCount(const std::string &table);

  private:
    /** 锁表：table -> 等待队列。懒创建（第一次访问时插入） */
    std::unordered_map<std::string, LockRequestQueue> lock_table_;
    std::mutex table_lock_;
};

}  // namespace hdb
