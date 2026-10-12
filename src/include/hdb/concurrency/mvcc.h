#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#include "hdb/common/rid.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

/**
 * @file mvcc.h
 * @brief Lab 6（选做）：MVCC 多版本并发控制。
 *
 * 与 2PL 的"阻塞换隔离"不同，MVCC 用"多版本"换并发：
 * 写者追加新版本，读者按快照时间戳读旧版本，读写互不阻塞。
 *
 * 三个概念（建议先读 docs/lab6-mvcc.md）：
 *   * read_ts：事务 Begin 时拿到的时间戳眼镜——只能看见 commit_ts <= read_ts 的版本；
 *   * commit_ts：Commit 时单调递增分配，作为新版本的生效时间；
 *   * 版本链：挂在同一 RID 上的历史版本序列（新 → 旧），
 *     每个版本带 (commit_ts, is_deleted, tuple)；墓碑也是一种版本。
 *
 * 教学版语义边界（写进实验报告的讨论素材）：
 *   * 只考虑已提交版本的可见性（未提交写入的暂存机制可参考
 *     BusTub 的 TXN_START_ID 偏移技巧，作为拓展）；
 *   * 快照读不加任何锁——这正是读写不阻塞的来源。
 */

/** 版本链上的一个历史版本 */
struct VersionRecord {
    uint64_t commit_ts{0};  ///< 该版本的提交时间戳
    bool is_deleted{false}; ///< 墓碑版本：删除对快照生效的标记
    Tuple tuple;            ///< 该版本的元组内容（墓碑时无意义）
};

class MvccManager {
  public:
    /**
     * TODO(Lab6): 分配读时间戳 = 最近一次已提交的 commit_ts。
     * （眼镜戴上的瞬间：此后别人的提交都看不见）
     */
    uint64_t BeginReadTimestamp();

    /**
     * TODO(Lab6): 分配提交时间戳（串行化自增，返回新值）。
     */
    uint64_t CommitTimestamp();

    /**
     * TODO(Lab6): 按 read_ts 回放版本链，重建本事务应看到的版本。
     * @param versions 某 RID 的版本链，**按新 → 旧**排列
     * @param out 输出可见版本
     * @return false 表示该记录对本快照不可见（从未存在或已删除）
     */
    bool ReconstructTuple(uint64_t read_ts, const std::vector<VersionRecord> &versions, Tuple *out);

    /**
     * TODO(Lab6): 单个版本的可见性判断：
     * 版本的提交时间不晚于快照（commit_ts <= read_ts）且不是墓碑 → 可见。
     */
    bool IsVisible(uint64_t read_ts, uint64_t version_ts, bool is_deleted);

    uint64_t LastCommitTs() const { return last_commit_ts_; }

  private:
    uint64_t last_commit_ts_{0};
    std::mutex ts_lock_;  // 串行化时间戳分配
};

}  // namespace hdb
