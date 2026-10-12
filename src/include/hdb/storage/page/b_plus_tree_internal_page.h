#pragma once

#include <utility>

#include "hdb/common/config.h"
#include "hdb/storage/index/generic_key.h"
#include "hdb/storage/page/b_plus_tree_page.h"

namespace hdb {

/**
 * @file b_plus_tree_internal_page.h
 * @brief Lab 3：B+ 树内部页——只做导航，不存数据。
 *
 * 布局（紧跟 20B 公共头）：
 *
 *   [公共头 20B][ array[0] ][ array[1] ][ ... ][ array[size] ]
 *                  │           │
 *   array[0].first = 无效键   array[1] = (key1, child_page_id1)
 *        （首键占位，永不读取）  key_i 是"进入 child_i 的最小键下界"
 *
 * 不变式（Invariant，你的实现必须维护；与 BusTub 一致）：
 *   size_ = 数组条目数（含 array[0] 占位项）= 子页数；有效键位于 [1, size-1]；
 *   array[i].second 承接键落在 [key_i, key_{i+1}) 区间的子页。
 * 查找语义：Lookup(key) 返回**第一个满足 key < array[i].first** 的左邻子页。
 *
 * @note 页面类只能有"平凡可构造"的字段（禁止 vector/string）——
 *       它们活在 4KB 裸字节里（这也是 BusTub 的硬性约束）。
 */
class BPlusTreeInternalPage : public BPlusTreePage {
  public:
    using MappingType = std::pair<IndexKey, page_id_t>;
    static constexpr uint32_t ARRAY_OFFSET = 20;

    /** 单个 (key, child) 占用的字节数（IndexKey 定长 8 + page_id 4） */
    static constexpr uint32_t EntrySize() { return sizeof(MappingType); }

    /** 本页能容纳的最大键数（给定） */
    static int MaxCapacity() { return (PAGE_SIZE - ARRAY_OFFSET) / EntrySize(); }

    // -----------------------------------------------------------------------
    // 以下为 Lab3 填空区
    // -----------------------------------------------------------------------

    /**
     * TODO(Lab3): 初始化内部页（首键占位 + size=0）。
     * @param parent 新根分裂时的父页（平常为 INVALID_PAGE_ID）
     */
    void Init(page_id_t page_id, page_id_t parent = INVALID_PAGE_ID);

    /**
     * TODO(Lab3): 二分查找 key 应当走向的子页。
     * @param comparator 数值比较器（-1/0/1 语义同 strcmp）
     * @return 子页 page_id
     */
    page_id_t Lookup(const IndexKey &key, const NumericComparator<8> &comparator) const;

    /** TODO(Lab3): 读/写第 index 个键与值（越界属于实现 bug，assert 掉） */
    IndexKey KeyAt(int index) const;
    void SetKeyAt(int index, const IndexKey &key);
    page_id_t ValueAt(int index) const;

    /**
     * TODO(Lab3): 分裂出新的根（第一次叶子分裂时使用）。
     * 用 (old_root/new_node, old_root/new_node) 两条导航项填满本页（此时本页是根）。
     */
    void PopulateNewRoot(page_id_t old_node, const IndexKey &new_key, page_id_t new_node);

    /**
     * TODO(Lab3): 在 array[index] 之后插入 (key, value)，size++。
     * @return 插入后的 size
     */
    int InsertNodeAfter(int index, const IndexKey &key, page_id_t value);

    /** TODO(Lab3): 删除 array[index] 整项（保持其余项连续），size--。 */
    void Remove(int index);

    /**
     * TODO(Lab3): 分裂辅助——把本页后半段 MoveHalfTo(receiver)。
     * @param receiver 新建的右兄弟页（已 Init）
     */
    void MoveHalfTo(BPlusTreeInternalPage *receiver);

    /**
     * TODO(Lab3): 重分配辅助——把本页最后一项挪给 receiver 的第一项。
     * 别忘了 receiver 新首键上推给公共父页（由 BPlusTree 层调用 SetKeyAt 完成）。
     */
    void MoveLastToFrontOf(BPlusTreeInternalPage *receiver);

    /** TODO(Lab3): 重分配辅助——把本页第一项挪给 receiver 的末尾。 */
    void MoveFirstToEndOf(BPlusTreeInternalPage *receiver);

  private:
    /** 定长条目数组的裸起点（给定） */
    MappingType *Array() { return reinterpret_cast<MappingType *>(reinterpret_cast<char *>(this) + ARRAY_OFFSET); }
    const MappingType *Array() const {
        return reinterpret_cast<const MappingType *>(reinterpret_cast<const char *>(this) + ARRAY_OFFSET);
    }

    // 以下搬运辅助供 Move* 系列复用（学生可自由改造）
    void CopyFrom(MappingType *items, int count);       // 批量追加到尾部
    void InsertFront(const MappingType &entry);         // 前插一条（整体后移）
    void AppendBack(const MappingType &entry);          // 追加一条
};

}  // namespace hdb
