#pragma once

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

class TableHeap;

/**
 * @file table_iterator.h
 * @brief Lab 2 Task 4：表迭代器——遍历整张表的唯一姿势。
 *
 * 执行器（Lab4 的 SeqScan）拿到的就是它：
 *
 *   for (auto it = heap.Begin(); it != heap.End(); ++it) {
 *       Tuple row = *it;   // 逐行拉取
 *   }
 *
 * 语义要求：
 *   * 跳过墓碑记录（对上层"删除即不可见"）；
 *   * 跨页推进：本页耗尽 → 沿 next_page_id 跳页；
 *   * 与 End() 相等的迭代器是"尾后"（dereference 非法）；
 *   * 拷贝语义（BusTub 风格：迭代器是值对象，内部管理页引用计数）。
 */
class TableIterator {
  public:
    /**
     * @param heap 被遍历的表（不拥有）
     * @param rid  当前位置（INVALID 表示尾后）
     */
    TableIterator(const TableHeap *heap, RID rid);

    ~TableIterator();

    TableIterator(const TableIterator &other);
    TableIterator &operator=(const TableIterator &other);

    /** TODO(Lab2): 解引用出当前记录（当前 rid 上的 tuple） */
    Tuple operator*() const;

    /** TODO(Lab2): 前进到下一条存活记录（跨页 + 跳墓碑），到尾则置尾后态 */
    TableIterator &operator++();

    bool operator==(const TableIterator &other) const { return rid_ == other.rid_; }
    bool operator!=(const TableIterator &other) const { return !(*this == other); }

    const RID &GetRid() const { return rid_; }

  private:
    const TableHeap *heap_;
    RID rid_{};
};

}  // namespace hdb
