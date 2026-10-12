#pragma once

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/index/generic_key.h"
#include "hdb/storage/page/page.h"

namespace hdb {

class BufferPoolManager;
class BPlusTreeLeafPage;

/**
 * @file index_iterator.h
 * @brief Lab 3：索引迭代器——沿叶子链表做范围扫描。
 *
 *   for (auto it = tree.Begin(key); it != tree.End(); ++it) {
 *       RID rid = *it;   // 逐键拉取，天然有序
 *   }
 *
 * 持有状态：当前叶子页（Page 指针，经缓冲池 pin 住）+ 页内下标。
 * 到达页尾 → 沿 next_page_id 换页（旧页 unpin，新页 pin）。
 * 到达链尾 → 变成 End() 等价的尾后迭代器。
 */
class IndexIterator {
  public:
    IndexIterator() = default;

    /**
     * @param bpm      缓冲池（负责页的进出）
     * @param leaf_page 已 pin 的叶子页（nullptr 表示尾后）
     * @param index    页内起始下标
     */
    IndexIterator(BufferPoolManager *bpm, Page *leaf_page, int index);

    ~IndexIterator();

    IndexIterator(const IndexIterator &other);
    IndexIterator &operator=(const IndexIterator &other);

    IndexIterator(IndexIterator &&other) noexcept;
    IndexIterator &operator=(IndexIterator &&other) noexcept;

    /** 当前键（尾后迭代器调用属于 bug，assert 掉） */
    IndexKey Key() const;

    /** 当前 RID */
    RID operator*() const;

    /** TODO(Lab3): 前进到下一键；页尾则换页；链尾则进入尾后态 */
    IndexIterator &operator++();

    bool IsEnd() const { return leaf_page_ == nullptr; }

    bool operator==(const IndexIterator &other) const {
        return leaf_page_ == other.leaf_page_ && index_ == other.index_;
    }
    bool operator!=(const IndexIterator &other) const { return !(*this == other); }

  private:
    BufferPoolManager *bpm_{nullptr};
    Page *leaf_page_{nullptr};  // 由本迭代器持有 pin 的叶子帧
    int index_{0};
};

}  // namespace hdb
