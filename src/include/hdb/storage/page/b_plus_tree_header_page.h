#pragma once

#include "hdb/common/config.h"
#include "hdb/storage/page/b_plus_tree_page.h"

namespace hdb {

/**
 * @brief B+ 树头页（框架给定）——整棵树的"入口牌"。
 *
 * 只存一件事：根页号。为什么要单独一页？
 *   * 根会因分裂/合并而更换页号，所有路径都要能找到"当前的根"；
 *   * 把可变的 root_page_id 固定在一个永不变的页号上，
 *     BPlusTree 对象（内存里）崩溃重建后仍能找回树。
 */
class BPlusTreeHeaderPage : public BPlusTreePage {
  public:
    static constexpr uint32_t ROOT_PAGE_ID_OFFSET = 20;  // 紧跟公共头

    void Init(page_id_t header_page_id) {
        SetPageType(IndexPageType::HEADER);
        SetSize(0);
        SetMaxSize(0);
        SetParentPageId(INVALID_PAGE_ID);
        SetPageId(header_page_id);
        SetRootPageId(INVALID_PAGE_ID);  // 尚无根 = 空树
    }

    page_id_t GetRootPageId() const {
        return *reinterpret_cast<const page_id_t *>(reinterpret_cast<const char *>(this) +
                                                    ROOT_PAGE_ID_OFFSET);
    }

    void SetRootPageId(page_id_t root_page_id) {
        *reinterpret_cast<page_id_t *>(reinterpret_cast<char *>(this) + ROOT_PAGE_ID_OFFSET) =
            root_page_id;
    }
};

}  // namespace hdb
