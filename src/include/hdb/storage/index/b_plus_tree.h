#pragma once

#include <queue>
#include <string>
#include <vector>

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/index/generic_key.h"
#include "hdb/storage/index/index.h"
#include "hdb/storage/index/index_iterator.h"
#include "hdb/storage/page/page_guard.h"
#include "hdb/storage/page/b_plus_tree_header_page.h"
#include "hdb/storage/page/b_plus_tree_internal_page.h"
#include "hdb/storage/page/b_plus_tree_leaf_page.h"

namespace hdb {

class InternalPageAlias;
class LeafPageAlias;

/**
 * @file b_plus_tree.h
 * @brief Lab 3：B+ 树主体——把三个页面类组织成一棵会长大也会缩小的树。
 *
 * 模块关系：
 *
 *   BPlusTree ──► HeaderPage（入口牌：root_page_id）
 *        │
 *        ├──► InternalPage（导航，键数 = 子页数 - 1）
 *        └──► LeafPage（数据 + 横向链表）──► IndexIterator（范围扫描）
 *
 * 你要实现的三个入口 + 并发：
 *   Insert   自顶向下（写蟹行）找到叶子插入；满则分裂、中间键上推，
 *            根分裂时换根（写 HeaderPage）
 *   GetValue 自顶向下（读蟹行）到叶子查找
 *   Remove   到叶子删除；欠载时先试兄弟重分配、再试与兄弟合并，
 *            并向上传播（父页删键、可能连锁合并）
 *   并发要求（必做）：**乐观蟹行加锁**（crabbing）——
 *            下降时锁子页，确认"安全"（不会再分裂/合并）后立即放父页；
 *            禁止全程持有根锁（那样等于把树串行化）。
 *
 * 调试工具：tools/b_plus_tree_printer 把整棵树导出为 DOT 图。
 */
class BPlusTree : public Index {
    using InternalPage = BPlusTreeInternalPage;
    using LeafPage = BPlusTreeLeafPage;
    using HeaderPage = BPlusTreeHeaderPage;

  public:
    /**
     * @param bpm          缓冲池（树页全部经它访问）
     * @param key_type     键的数值类型（喂给比较器）
     * @param name         索引名（决定 header 页的登记）
     */
    BPlusTree(BufferPoolManager *bpm, TypeId key_type, std::string name, Schema key_schema);

    ~BPlusTree() override = default;

    /**
     * TODO(Lab3): 建立/找回这棵树：分配（或读取）header 页，记录 header_page_id_。
     * @note 空树 = header 里 root 为 INVALID_PAGE_ID。
     */
    void Init();

    // ---- Index 接口的三个入口（Lab3 主战场） ----

    /** TODO(Lab3): 插入 (key, rid)。重复键返回 false（唯一索引语义）。 */
    bool InsertEntry(const IndexKey &key, const RID &rid) override;

    /** TODO(Lab3): 精确查找，结果追加进 result。 */
    void ScanKey(const IndexKey &key, std::vector<RID> *result) override;

    /** TODO(Lab3): 读蟹行下降到叶子查找；命中写入 *result 返回 true。 */
    bool GetValue(const IndexKey &key, RID *result);

    /** TODO(Lab3): 删除键。键不存在返回 false。 */
    bool DeleteEntry(const IndexKey &key, const RID &rid) override;

    // ---- 迭代（给定签名，行为在 IndexIterator 上实现） ----

    /** TODO(Lab3): 定位到第一个键 >= key 的迭代器（范围扫描左端点） */
    IndexIterator Begin(const IndexKey &key);

    /** TODO(Lab3): 指向树中第一个键 */
    IndexIterator Begin();

    IndexIterator End();

    // ---- 观察内部状态（printer / 测试用） ----

    /** TODO(Lab3): 经 HeaderPage 读当前根页号 */
    page_id_t GetRootPageId();

    /** 给定实现：整棵树的文本画像（节点: 键序列），printer 与测试共用 */
    std::string DrawBPlusTree();

    bool IsEmpty();

    BufferPoolManager *GetBufferPoolManager() { return bpm_; }
    const NumericComparator<8> &GetComparator() const { return comparator_; }

  private:
    /** 换根：把 new_root 写进 HeaderPage（分裂 / 收缩时调用；调用方持 header 写锁） */
    void UpdateRoot(page_id_t new_root);

    /** InsertEntry/DeleteEntry 的锁内实现（header 写锁由外层方法持有） */
    bool InsertEntryLocked(const IndexKey &key, const RID &rid);
    bool DeleteEntryLocked(const IndexKey &key, const RID &rid);

    BufferPoolManager *bpm_;
    NumericComparator<8> comparator_;
    page_id_t header_page_id_{INVALID_PAGE_ID};

    // ---- 其余内部辅助由学生按需增删 ----
    // FindLeaf(key, mode)          蟹行下降，返回叶子页守卫
    // Split(leaf / internal)       分裂 + 中间键上推
    // CoalesceOrRedistribute(node) 欠载处理（重分配优先于合并）
};

}  // namespace hdb
