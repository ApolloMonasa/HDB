#pragma once

#include <cstdint>

#include "hdb/common/config.h"

namespace hdb {

/** B+ 树页面类型标记（存在每个树页的头部） */
enum class IndexPageType : int32_t { INVALID = 0, HEADER = 1, INTERNAL = 2, LEAF = 3 };

/**
 * @file b_plus_tree_page.h
 * @brief B+ 树页面的公共头部（框架给定）。
 *
 * 三种树页（Header/Internal/Leaf）都把下面这 20 字节摆在页首：
 *
 *   [page_type 4B][size 4B][max_size 4B][parent_page_id 4B][page_id 4B]
 *
 * 因此 BPlusTree 在蟹行下降时，拿到子页后**先读公共头**判断
 * "到叶子了吗"，再 reinterpret_cast 成具体页面类型——
 * 这是 crabbing 循环里最关键的一次类型分派。
 */
class BPlusTreePage {
  public:
    virtual ~BPlusTreePage() = default;

    bool IsLeafPage() const { return GetPageType() == IndexPageType::LEAF; }
    bool IsInternalPage() const { return GetPageType() == IndexPageType::INTERNAL; }
    bool IsHeaderPage() const { return GetPageType() == IndexPageType::HEADER; }

    IndexPageType GetPageType() const { return *reinterpret_cast<const IndexPageType *>(GetData()); }
    void SetPageType(IndexPageType type) { *reinterpret_cast<IndexPageType *>(GetData()) = type; }

    /** 当前键数量（internal 页的"键数 = 子页数 - 1"另见各页说明） */
    int GetSize() const { return *reinterpret_cast<const int32_t *>(GetData() + 4); }
    void SetSize(int size) { *reinterpret_cast<int32_t *>(GetData() + 4) = size; }
    void IncreaseSize(int amount) { *reinterpret_cast<int32_t *>(GetData() + 4) += amount; }

    /** 键容量上限（**达到即视为满**——为分裂前的整理留出余量） */
    int GetMaxSize() const { return *reinterpret_cast<const int32_t *>(GetData() + 8); }
    void SetMaxSize(int max_size) { *reinterpret_cast<int32_t *>(GetData() + 8) = max_size; }

    /** 键数量下限（低于它删除后要合并/重分配；通常 = max_size/2） */
    int GetMinSize() const { return GetMaxSize() / 2; }

    page_id_t GetParentPageId() const {
        return *reinterpret_cast<const page_id_t *>(GetData() + 12);
    }
    void SetParentPageId(page_id_t parent) {
        *reinterpret_cast<page_id_t *>(GetData() + 12) = parent;
    }

    page_id_t GetPageId() const { return *reinterpret_cast<const page_id_t *>(GetData() + 16); }
    void SetPageId(page_id_t page_id) { *reinterpret_cast<page_id_t *>(GetData() + 16) = page_id; }

  private:
    /** 4KB 数据区首地址（子类沿用"reinterpret_cast 到 Page 数据上"的用法） */
    char *GetData() { return reinterpret_cast<char *>(this); }
    const char *GetData() const { return reinterpret_cast<const char *>(this); }
};

}  // namespace hdb
