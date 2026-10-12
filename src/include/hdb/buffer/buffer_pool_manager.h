#pragma once

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "hdb/buffer/lru_k_replacer.h"
#include "hdb/buffer/replacer.h"
#include "hdb/common/config.h"

namespace hdb {

class DiskManager;
class DiskScheduler;
class Page;

/**
 * @file buffer_pool_manager.h
 * @brief Lab 1 Task 3：缓冲池管理器——HDB 的"内存侧总闸"。
 *
 * 上层（表堆、B+ 树）对"页"的全部诉求都汇总到这七个方法：
 *
 *   FetchPage  把磁盘页调进内存（命中则只动 pin 计数与替换器）
 *   NewPage    分配新页（先问替换器要空闲帧；没有可驱逐帧返回 nullptr）
 *   UnpinPage  用完归还（pin 计数减一；is_dirty 累计脏标记）
 *   FlushPage  单页落盘    FlushAllPages 全部落盘
 *   DeletePage 删页（必须 pin 计数为 0 才能删，同时通知磁盘释放页号）
 *
 * 内部数据结构（三个"登记簿"）：
 *
 *   pages_      帧数组（固定 pool_size 个 Page 对象，复用不释放）
 *   page_table_ page_id -> frame_id 的映射（页表）
 *   free_list_  从未使用过的帧；耗尽后靠 replacer_ 腾帧
 *
 * 驱逐一个脏页的完整动作（务必按序）：
 *   写回磁盘 → 从 page_table_ 摘除旧 page_id → 换上新 page_id → 清零数据 → 置脏标记
 */
class BufferPoolManager {
  public:
    /**
     * @param pool_size     帧数
     * @param disk_manager  磁盘管理器（不转移所有权）
     * @param replacer_k    LRU-K 的 K
     */
    BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k = 10);

    ~BufferPoolManager();

    BufferPoolManager(const BufferPoolManager &) = delete;
    BufferPoolManager &operator=(const BufferPoolManager &) = delete;

    /**
     * TODO(Lab1): 取页。已在池中：pin++ 并 SetEvictable(false)；
     * 不在池中：腾一个帧（先 free_list_ 后 replacer_->Evict，脏页先写回），
     * 经 disk_scheduler_ 异步读入。
     * @return 失败（页不存在 / 无帧可用）返回 nullptr
     */
    Page *FetchPage(page_id_t page_id);

    /**
     * TODO(Lab1): 新建页。流程与 FetchPage 的缺页路径相同，
     * 只是数据来源从"读磁盘"换成"清零"。
     * @param page_id 输出新页号（由 disk_manager_->IncreaseNumPages() 分配）
     */
    Page *NewPage(page_id_t *page_id);

    /**
     * TODO(Lab1): 归还页。pin 计数减到 0 时 SetEvictable(true)。
     * is_dirty 与页上已有脏标记取"或"。
     * @return 页不在池中返回 false
     */
    bool UnpinPage(page_id_t page_id, bool is_dirty);

    /**
     * TODO(Lab1): 把指定页写回磁盘（无论是否脏、是否被 pin）。
     * 写回后清脏标记。@return 页不在池中返回 false
     */
    bool FlushPage(page_id_t page_id);

    /**
     * TODO(Lab1): 删除页。pin 计数非 0 的页不能删（返回 nullptr）。
     * 删除 = 从页表摘除 + 归还帧（free_list_ 或 replacer_->Remove）
     *        + disk_manager_->DeallocatePage(page_id)。
     * @return 成功删除时返回该帧上的 Page 指针（供上层清结构），否则 nullptr
     */
    Page *DeletePage(page_id_t page_id);

    /** TODO(Lab1): 池内全部页落盘（正常关库路径） */
    void FlushAllPages();

    size_t GetPoolSize() const { return pool_size_; }

    /**
     * 守卫工厂（Lab1 Task4 之后推荐的取页姿势，替代裸 FetchPage）：
     *   auto guard = bpm.ReadPageGuardFor(page_id);
     */
    class ReadPageGuard ReadPageGuardFor(page_id_t page_id);    // NOLINT：见 page_guard.cpp
    class WritePageGuard WritePageGuardFor(page_id_t page_id);  // NOLINT

    /** 测试辅助：页是否当前驻留在池中 */
    bool InPool(page_id_t page_id) const {
        std::lock_guard<std::mutex> lk(latch_);
        return page_table_.find(page_id) != page_table_.end();
    }

  protected:
    /** 腾出一个可用帧（free_list_ 优先，其次驱逐），返回帧号；失败返回 false */
    // TODO(Lab1): 这个辅助函数是强烈建议的拆分点，驱逐的完整动作都在这里
    bool FindFreeFrame(frame_id_t *frame_id);

    size_t pool_size_;
    Page *pages_;
    DiskManager *disk_manager_;
    std::unique_ptr<DiskScheduler> disk_scheduler_;
    std::unique_ptr<Replacer> replacer_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::list<frame_id_t> free_list_;
    mutable std::mutex latch_;

    friend class PageGuardTest;  // 测试需要窥视内部状态
};

}  // namespace hdb
