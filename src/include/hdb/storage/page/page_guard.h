#pragma once

#include "hdb/common/config.h"

namespace hdb {

class BufferPoolManager;
class Page;

/**
 * @file page_guard.h
 * @brief Lab 1 Task 4：页守卫（RAII）——缓冲池的手套和口罩。
 *
 * 直接操作 Page* 的代码有三种经典死法：
 *   1. 忘了 Unpin → 帧永远驻留，池子慢慢被钉死；
 *   2. 忘了置脏 → 修改过的页被无警告驱逐，数据丢失；
 *   3. 忘了解锁 → 下一个线程在页锁上挂到天荒地老。
 *
 * 守卫把这三件事绑定到 C++ 的**析构函数**上——异常路径也能全身而退：
 *
 *   {
 *     WritePageGuard guard = bpm->WritePageGuardFor(page_id);  // 锁页 + pin
 *     guard->GetData()[0] = 42;                                 // 用页
 *   }                                                           // 自动：解锁 + 置脏 + unpin
 *
 * 三个类的分工：
 *   BasicPageGuard   只管"unpin 时是否置脏 + 析构 Drop"（BPM 需要它做构造中转）
 *   ReadPageGuard    构造 RLock / Drop 时 RUnlock + unpin(false)
 *   WritePageGuard   构造 WLock / Drop 时 WUnlock + unpin(true)
 */
class BasicPageGuard {
  public:
    BasicPageGuard() = default;

    /** @param bpm 拥有该页的缓冲池  @param page 被守卫的页（已 pin） */
    BasicPageGuard(BufferPoolManager *bpm, Page *page);

    virtual ~BasicPageGuard();

    BasicPageGuard(const BasicPageGuard &) = delete;
    BasicPageGuard &operator=(const BasicPageGuard &) = delete;

    /**
     * TODO(Lab1): 移动构造——把 other 的资源据为己有，把 other 置空。
     * @note  被移动后的守卫不得在析构时 Drop（否则双重 unpin）。
     */
    BasicPageGuard(BasicPageGuard &&other) noexcept;

    /** TODO(Lab1): 移动赋值——先 Drop 自己再接收（自赋值要安全） */
    BasicPageGuard &operator=(BasicPageGuard &&other) noexcept;

    /** TODO(Lab1): 立即归还页（等价于析构动作），之后本守卫为空 */
    void Drop();

    /** 像指针一样使用：guard->GetData() */
    Page *operator->() const { return page_; }

    Page *GetPage() const { return page_; }

  protected:
    BufferPoolManager *bpm_{nullptr};
    Page *page_{nullptr};
    bool is_dirty_{false};
};

/** 读守卫：多读并存 */
class ReadPageGuard : public BasicPageGuard {
  public:
    ReadPageGuard() = default;

    /**
     * TODO(Lab1): 构造时对 page_ 上读锁。
     * 提示：BPM 返回的是已 pin 的 Page*，上锁动作在守卫构造里完成。
     */
    ReadPageGuard(BufferPoolManager *bpm, Page *page);

    ~ReadPageGuard() override;

    ReadPageGuard(ReadPageGuard &&other) noexcept;
    ReadPageGuard &operator=(ReadPageGuard &&other) noexcept;

    /** TODO(Lab1): 解读锁 + UnpinPage(false)。 */
    void Drop();

  private:
    bool locked_{false};  // 移动语义下追踪"锁是否还在我手上"
};

/** 写守卫：独占写，Drop 时置脏 */
class WritePageGuard : public BasicPageGuard {
  public:
    WritePageGuard() = default;

    /** TODO(Lab1): 构造时对 page_ 上写锁 */
    WritePageGuard(BufferPoolManager *bpm, Page *page);

    ~WritePageGuard() override;

    WritePageGuard(WritePageGuard &&other) noexcept;
    WritePageGuard &operator=(WritePageGuard &&other) noexcept;

    /** TODO(Lab1): 解写锁 + UnpinPage(page_id, true)。 */
    void Drop();

  private:
    bool locked_{false};
};

}  // namespace hdb
