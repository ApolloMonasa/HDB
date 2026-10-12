#pragma once

#include <atomic>
#include <cstring>

#include "hdb/common/config.h"
#include "hdb/common/rwlatch.h"

namespace hdb {

/**
 * @brief 缓冲池中的物理页（框架给定，无需修改）。
 *
 * 这是"内存里的一个 4KB 页"的唯一表示：一段裸字节 + 页簿记信息。
 * TablePage、B+ 树页等上层结构全部通过 reinterpret_cast 把 data_
 * 解释成自己的页面头与数据布局——因此它们"住在"缓冲池里，
 * 而不是自己 new 出来（这正是 Lab1 建立的铁律）。
 *
 * 页簿记三件套：
 *   page_id_   本页对应的磁盘页号（INVALID 表示尚未关联磁盘）
 *   pin_count_ 正在使用本页的"人次"；>0 时替换器不得驱逐本页
 *   is_dirty_  自从读入/新建以来是否被改过（脏页驱逐前必须写回）
 */
class Page {
  public:
    Page() { ResetMemory(); }

    ~Page() = default;
    Page(const Page &) = delete;
    Page &operator=(const Page &) = delete;

    /** 页内裸数据（上层页面结构的载体） */
    char *GetData() { return data_; }
    const char *GetData() const { return data_; }

    page_id_t GetPageId() const { return page_id_; }
    void SetPageId(page_id_t id) { page_id_ = id; }

    int GetPinCount() const { return pin_count_; }
    int IncrementPinCount() { return ++pin_count_; }
    int DecrementPinCount() { return --pin_count_; }

    bool IsDirty() const { return is_dirty_; }
    void SetDirty(bool dirty) { is_dirty_ = dirty; }

    RWLatch &GetLatch() { return latch_; }

    /** 把整页清零（新页使用前调用，避免读到上一个页面的残影） */
    void ResetMemory() { std::memset(data_, 0, PAGE_SIZE); }

  private:
    char data_[PAGE_SIZE] = {};
    page_id_t page_id_ = INVALID_PAGE_ID;
    int pin_count_ = 0;
    bool is_dirty_ = false;
    RWLatch latch_;
};

}  // namespace hdb
