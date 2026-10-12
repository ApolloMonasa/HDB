#pragma once

#include <mutex>
#include <shared_mutex>

namespace hdb {

/**
 * @brief 读写锁的极简封装（框架给定）。
 *
 * 页面是典型的"读多写少"数据结构：并发扫描时多个线程可以同时持有读锁，
 * 写页面（分裂、修改记录）时必须独占。C++17 的 std::shared_mutex
 * 已经提供了全部能力，这里封装一层是为了让加锁意图在代码里更醒目，
 * 也便于将来替换成更高级的锁实现（如优先级读写锁）。
 *
 * 使用守则（数据库内核版）：
 *   - 沿着 B+ 树向下蟹行时，子页加锁成功后立即释放父页的锁；
 *   - 拿锁顺序必须单向，否则极易死锁。
 */
class RWLatch {
  public:
    /** 独占写锁（排他） */
    void WLock() { latch_.lock(); }
    void WUnlock() { latch_.unlock(); }

    /** 共享读锁（多读并存） */
    void RLock() { latch_.lock_shared(); }
    void RUnlock() { latch_.unlock_shared(); }

  private:
    std::shared_mutex latch_;
};

/**
 * @brief RAII 版读写锁守卫——避免"异常路径忘记解锁"这种最难查的 bug。
 *
 *   {
 *     ReaderGuard rg(page->latch());   // 构造加读锁
 *     ... 读页面 ...
 *   }                                  // 析构解锁，即使中间抛了异常
 */
class ReaderGuard {
  public:
    explicit ReaderGuard(RWLatch &l) : latch_(l) { latch_.RLock(); }
    ~ReaderGuard() { latch_.RUnlock(); }
    ReaderGuard(const ReaderGuard &) = delete;
    ReaderGuard &operator=(const ReaderGuard &) = delete;

  private:
    RWLatch &latch_;
};

/// 写者版 RAII 守卫
class WriterGuard {
  public:
    explicit WriterGuard(RWLatch &l) : latch_(l) { latch_.WLock(); }
    ~WriterGuard() { latch_.WUnlock(); }
    WriterGuard(const WriterGuard &) = delete;
    WriterGuard &operator=(const WriterGuard &) = delete;

  private:
    RWLatch &latch_;
};

}  // namespace hdb
