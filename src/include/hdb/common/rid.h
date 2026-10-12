#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>

#include "hdb/common/config.h"

namespace hdb {

/**
 * @brief 记录标识符（Record Identifier）：一行元组在整个数据库中的"地址"。
 *
 * 结构 = (页号, 页内槽位号)。索引（B+ 树）的叶子节点存放的就是 RID，
 * 通过 RID 可以直接定位到缓冲池里的那一页、那一个槽。
 *
 * 简单值语义：可拷贝、可比较、可哈希（框架给定，无需修改）。
 */
class RID {
  public:
    RID() = default;

    RID(page_id_t page_id, slot_id_t slot_id) : page_id_(page_id), slot_id_(slot_id) {}

    page_id_t GetPageId() const { return page_id_; }
    slot_id_t GetSlotId() const { return slot_id_; }

    void Set(page_id_t page_id, slot_id_t slot_id) {
        page_id_ = page_id;
        slot_id_ = slot_id;
    }

    bool operator==(const RID &other) const {
        return page_id_ == other.page_id_ && slot_id_ == other.slot_id_;
    }
    bool operator!=(const RID &other) const { return !(*this == other); }

    /// 输出形如 "(3,7)" —— 页 3 的槽 7
    friend std::ostream &operator<<(std::ostream &os, const RID &rid);

  private:
    page_id_t page_id_ = INVALID_PAGE_ID;
    slot_id_t slot_id_ = -1;
};

}  // namespace hdb

/// RID 的 std::hash 特化（unordered_map/set 可用）
template <>
struct std::hash<hdb::RID> {
    size_t operator()(const hdb::RID &rid) const {
        return std::hash<int64_t>()((static_cast<int64_t>(rid.GetPageId()) << 32) ^
                                    static_cast<uint32_t>(rid.GetSlotId()));
    }
};
