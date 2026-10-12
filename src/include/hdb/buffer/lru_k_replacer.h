#pragma once

#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "hdb/buffer/replacer.h"
#include "hdb/common/config.h"

namespace hdb {

/**
 * @file lru_k_replacer.h
 * @brief Lab 1 Task 1：LRU-K 替换器。
 *
 * 普通 LRU 只看"最近一次访问"，一条全表扫描就能把整个缓冲池的
 * 热数据冲掉（扫描污染）。LRU-K 用"最近 K 次访问"刻画一个页的
 * 访问模式，抗扫描污染能力显著更强——教材称"基于访问频次的改进 LRU"。
 *
 * ┌────────────────────────────────────────────────────────────────┐
 * │ 驱逐规则（考试会考，代码要一模一样）                             │
 * │                                                                │
 * │ backward k-distance(frame) = 当前时间 - 该帧第 K 次最近访问的时间  │
 * │   * 不足 K 次访问的帧：k-distance = +∞，优先被驱逐；             │
 * │     多个不足者之间按"最早一次访问"更早者先走（近似 LRU）          │
 * │   * 都有 K 次访问时：驱逐 backward k-distance 最大者             │
 * │   * 只有 SetEvictable(true) 的帧才有资格被选中                   │
 * └────────────────────────────────────────────────────────────────┘
 *
 * 实现要点：
 *   * 时间戳用全局自增计数器（std::atomic<uint64_t> 或锁内自增均可）；
 *   * 每帧维护一个长度 <= K 的访问时间队列；
 *   * 线程安全（一把大锁即可，性能优化不是本实验目标）；
 *   * 同一帧重复 RecordAccess 只是追加时间戳，不改变 evictable 状态。
 */
class LRUKReplacer : public Replacer {
  public:
    /**
     * @param num_frames 缓冲池帧数（帧号合法范围 [0, num_frames)）
     * @param k          LRU-K 的 K
     */
    explicit LRUKReplacer(size_t num_frames, size_t k);

    ~LRUKReplacer() override = default;

    /** TODO(Lab1): 按上面的驱逐规则选牺牲者；无可驱逐帧返回 false */
    bool Evict(frame_id_t *frame_id) override;

    /** TODO(Lab1): 记录一次访问。非法帧号直接忽略。 */
    void RecordAccess(frame_id_t frame_id) override;

    /**
     * TODO(Lab1): 调整帧的可驱逐状态并维护 curr_size_。
     * 从不可驱逐改为可驱逐时 curr_size_++；反向则--。
     * 非法帧号 / 帧已在替换器外（Remove 过）应如何处理？想想 assert 还是忽略。
     */
    void SetEvictable(frame_id_t frame_id, bool set_evictable) override;

    /** TODO(Lab1): 彻底移除一帧（调用方保证它已可驱逐），清空其历史 */
    void Remove(frame_id_t frame_id) override;

    /** TODO(Lab1): 返回可驱逐帧数量（注意不是全部帧数量） */
    size_t Size() const override;

  private:
    // TODO(Lab1): 你需要补充每帧的簿记结构。建议形如：
    //   struct FrameInfo { std::list<uint64_t> history; bool is_evictable; };
    //   std::unordered_map<frame_id_t, FrameInfo> frames_;
    //   mutable std::mutex latch_;
    size_t num_frames_;
    size_t k_;
    size_t curr_size_{0};
    mutable std::mutex latch_;  // mutable：const 版 Size() 也要读锁
};

}  // namespace hdb
