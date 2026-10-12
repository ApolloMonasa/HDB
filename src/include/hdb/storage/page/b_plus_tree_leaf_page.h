#pragma once

#include <utility>

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/index/generic_key.h"
#include "hdb/storage/page/b_plus_tree_page.h"

namespace hdb {

/**
 * @file b_plus_tree_leaf_page.h
 * @brief Lab 3：B+ 树叶子页——数据真正住的地方。
 *
 * 布局（紧跟 20B 公共头）：
 *
 *   [公共头 20B][ array[0] ][ array[1] ][ ... ][ array[size-1] ][next_page_id 4B]
 *                 (key, RID)   (key, RID)
 *
 * 叶子串成单向链表（next_page_id），范围扫描就是沿链游走——
 * 这是 B+ 树区别于 B 树的核心特征（教材：B+ 树支持高效顺序遍历）。
 *
 * 不变式：array 内键严格有序；size_ = 存活的键数。
 */
class BPlusTreeLeafPage : public BPlusTreePage {
  public:
    using MappingType = std::pair<IndexKey, RID>;
    static constexpr uint32_t ARRAY_OFFSET = 20;

    static int MaxCapacity() {
        // 尾部还要给 next_page_id 留 4 字节
        return (PAGE_SIZE - ARRAY_OFFSET - sizeof(page_id_t)) / sizeof(MappingType);
    }

    // -----------------------------------------------------------------------
    // 以下为 Lab3 填空区
    // -----------------------------------------------------------------------

    /** TODO(Lab3): 初始化叶子页；next_page_id 置 INVALID。 */
    void Init(page_id_t page_id, page_id_t parent = INVALID_PAGE_ID);

    page_id_t GetNextPageId() const;
    void SetNextPageId(page_id_t next);

    /**
     * TODO(Lab3): 二分查找——第一个 >= key 的下标。
     * （Insert 与 KeyIndex 共用；IndexIterator 的定位也靠它）
     */
    int KeyIndex(const IndexKey &key, const NumericComparator<8> &comparator) const;

    IndexKey KeyAt(int index) const;
    RID RidAt(int index) const;

    /**
     * TODO(Lab3): 插入 (key, rid) 保持有序；重复键返回 -1（唯一索引）。
     * @return 插入后的 size；键已存在返回 -1
     */
    int Insert(const IndexKey &key, const RID &rid, const NumericComparator<8> &comparator);

    /**
     * TODO(Lab3): 定位查找——命中的 rid 写入 *result 返回 true。
     */
    bool Lookup(const IndexKey &key, RID *result, const NumericComparator<8> &comparator) const;

    /** TODO(Lab3): 删除键等于 key 的项；不存在返回 -1，否则返回删除后的 size。 */
    int RemoveAndDeleteTuple(const IndexKey &key, const NumericComparator<8> &comparator);

    /** TODO(Lab3): 分裂辅助——把后半段搬到 receiver，并把 next 链接给 receiver。 */
    void MoveHalfTo(BPlusTreeLeafPage *receiver);

    /** TODO(Lab3): 合并辅助——把本页全部条目搬到 receiver 末尾（receiver 随后被释放）。 */
    void MoveAllTo(BPlusTreeLeafPage *receiver);

    /**
     * TODO(Lab3): 重分配辅助——把本页最后一项挪给 receiver 第一项（前插）。
     */
    void MoveLastToFrontOf(BPlusTreeLeafPage *receiver);

    /** TODO(Lab3): 重分配辅助——把本页第一项挪给 receiver 末尾（追加）。 */
    void MoveFirstToEndOf(BPlusTreeLeafPage *receiver);

  private:
    // 搬运辅助（学生可自由改造）
    void AppendRange(MappingType *items, int count);  // 批量追加到尾部
    void InsertFront(const MappingType &entry);       // 前插一条

    MappingType *Array() { return reinterpret_cast<MappingType *>(reinterpret_cast<char *>(this) + ARRAY_OFFSET); }
    const MappingType *Array() const {
        return reinterpret_cast<const MappingType *>(reinterpret_cast<const char *>(this) + ARRAY_OFFSET);
    }
    page_id_t *NextPageIdPtr() {
        return reinterpret_cast<page_id_t *>(reinterpret_cast<char *>(this) + PAGE_SIZE - sizeof(page_id_t));
    }
    const page_id_t *NextPageIdPtr() const {
        return reinterpret_cast<const page_id_t *>(reinterpret_cast<const char *>(this) + PAGE_SIZE -
                                                   sizeof(page_id_t));
    }
};

}  // namespace hdb
