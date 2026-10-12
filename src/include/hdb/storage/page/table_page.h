#pragma once

#include <cstdint>

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

/**
 * @file table_page.h
 * @brief Lab 2 Task 2：表页——"关系表的一行"住进 4KB 页面后的样子。
 *
 * 这是数据库存储里最经典的页面布局（教材称 slotted page / 槽页）：
 *
 *   低地址                                            高地址
 *   ┌──────────────┬────────────────┬────────────────┐
 *   │   表页头 16B  │ 槽位目录（向后长）│   数据区（向前长）│
 *   ├──────────────┼────────────────┼────────────────┤
 *   │ page_id  4B  │ slot0: {off,len}│   ← tuple N …  │
 *   │ prev_pid 4B  │ slot1: {off,len}│      …         │
 *   │ next_pid 4B  │ slot2: {off,len}│   tuple 0 →    │
 *   │ num_slots 4B │     （空闲隙）   │  （页内剩余空隙）│
 *   └──────────────┴────────────────┴────────────────┘
 *
 * 槽位目录从页头向后生长，数据区从页尾向前生长，中间的空隙就是
 * 本页剩余空间。变长记录插进数据区尾部，其 (offset, length) 登记
 * 成一个新槽位——第 i 条记录的地址就是 RID(page_id, i)。
 *
 * 删除采用**墓碑标记**（tombstone）：slot 的 offset 置 SLOT_DELETED，
 * 字节暂不回收（空间整理是思考题）；后续插入可复用墓碑槽。
 */
class TablePage {
  public:
    /** 表页头长度（5 个字段，见下方偏移常量） */
    static constexpr uint32_t TABLE_PAGE_HEADER_SIZE = 20;
    /** 槽位项大小：offset(2B) + length(2B) */
    static constexpr uint32_t SLOT_SIZE = 4;
    /** 墓碑标记：slot.offset 为该值表示记录已删除 */
    static constexpr uint16_t SLOT_DELETED = 0xFFFF;

    /**
     * 初始化一个全新的表页（框架给定）。
     * @param page_id 本页页号
     * @param prev_page_id 链表前驱（首页为 INVALID_PAGE_ID）
     */
    void Init(page_id_t page_id, page_id_t prev_page_id) {
        *reinterpret_cast<page_id_t *>(GetDataPtr() + OFFSET_PAGE_ID) = page_id;
        *reinterpret_cast<page_id_t *>(GetDataPtr() + OFFSET_PREV_PAGE_ID) = prev_page_id;
        *reinterpret_cast<page_id_t *>(GetDataPtr() + OFFSET_NEXT_PAGE_ID) = INVALID_PAGE_ID;
        *reinterpret_cast<int32_t *>(GetDataPtr() + OFFSET_NUM_SLOTS) = 0;
        *reinterpret_cast<uint16_t *>(GetDataPtr() + OFFSET_DATA_BEGIN) = PAGE_SIZE;  // 数据区空
    }

    // ---- 页面链表（框架给定） ----
    page_id_t GetTablePageId() const {
        return *reinterpret_cast<const page_id_t *>(GetDataPtr() + OFFSET_PAGE_ID);
    }
    page_id_t GetPrevPageId() const {
        return *reinterpret_cast<const page_id_t *>(GetDataPtr() + OFFSET_PREV_PAGE_ID);
    }
    page_id_t GetNextPageId() const {
        return *reinterpret_cast<const page_id_t *>(GetDataPtr() + OFFSET_NEXT_PAGE_ID);
    }
    void SetNextPageId(page_id_t next) {
        *reinterpret_cast<page_id_t *>(GetDataPtr() + OFFSET_NEXT_PAGE_ID) = next;
    }
    void SetPrevPageId(page_id_t prev) {
        *reinterpret_cast<page_id_t *>(GetDataPtr() + OFFSET_PREV_PAGE_ID) = prev;
    }

    // ---- 你要实现的槽位管理 ----

    /**
     * TODO(Lab2): 把 tuple 插入本页。
     * 1. 空间够吗？= 剩余空隙 >= tuple 长度 + （无墓碑可复用时）一个新槽位；
     * 2. 复用墓碑槽（长度足够）或追加新槽；
     * 3. tuple 字节拷到数据区尾部空隙的起点，登记 (offset, len)。
     * @param[out] rid 成功时写回记录地址
     * @return false 页已满（调用方应当新建下一页）
     */
    bool InsertTuple(const Tuple &tuple, RID *rid);

    /**
     * TODO(Lab2): 按 rid 取记录（墓碑记录返回 false）。
     * 反序列化交给 Tuple::DeserializeFrom。
     */
    bool GetTuple(const RID &rid, Tuple *tuple, const Schema &schema);

    /** TODO(Lab2): 墓碑删除。rid 非法或已删除返回 false。 */
    bool MarkDelete(const RID &rid);

    /**
     * TODO(Lab2): 原位更新（仅当新长度 <= 旧长度时成功，直接覆盖字节）。
     * 变长导致放不下时返回 false——由上层改走"删 + 插"。
     */
    bool UpdateTupleInPlace(const Tuple &tuple, const RID &rid);

    /**
     * TODO(Lab2): 迭代支持——给定一个 rid，返回本页内下一个"存活"记录的 rid；
     * 本页遍历完返回空 RID（调用方跳到 next_page_id）。
     */
    RID GetNextTupleRid(const RID &rid);

    /** 本页剩余可插入空间（字节）；给定实现，供你的 InsertTuple 判断 */
    uint32_t GetFreeSpaceSize() const {
        uint32_t num_slots = *reinterpret_cast<const int32_t *>(GetDataPtr() + OFFSET_NUM_SLOTS);
        uint32_t slot_area_end = TABLE_PAGE_HEADER_SIZE + num_slots * SLOT_SIZE;
        uint32_t data_area_begin = GetDataAreaBegin();
        return data_area_begin > slot_area_end ? data_area_begin - slot_area_end : 0;
    }

    /** 当前槽数（含墓碑） */
    uint32_t GetNumSlots() const {
        return *reinterpret_cast<const int32_t *>(GetDataPtr() + OFFSET_NUM_SLOTS);
    }

    /** 槽位是否为墓碑（给定） */
    bool IsDeleted(uint32_t slot_id) const { return GetSlotOffset(slot_id) == SLOT_DELETED; }

  private:
    // ---- 内部布局辅助（给定）：调用方负责保证 TablePage 挂在 Page 上 ----
    char *GetDataPtr();
    const char *GetDataPtr() const;

    uint16_t GetSlotOffset(uint32_t slot_id) const {
        return *reinterpret_cast<const uint16_t *>(GetDataPtr() + TABLE_PAGE_HEADER_SIZE +
                                                   slot_id * SLOT_SIZE);
    }
    uint16_t GetSlotLength(uint32_t slot_id) const {
        return *reinterpret_cast<const uint16_t *>(GetDataPtr() + TABLE_PAGE_HEADER_SIZE +
                                                   slot_id * SLOT_SIZE + 2);
    }
    void SetSlot(uint32_t slot_id, uint16_t offset, uint16_t length) {
        char *slot = GetDataPtr() + TABLE_PAGE_HEADER_SIZE + slot_id * SLOT_SIZE;
        *reinterpret_cast<uint16_t *>(slot) = offset;
        *reinterpret_cast<uint16_t *>(slot + 2) = length;
    }

    /** 数据区当前起点（下一条记录将紧贴它之前写入；随插入不断减小） */
    uint16_t GetDataAreaBegin() const {
        return *reinterpret_cast<const uint16_t *>(GetDataPtr() + OFFSET_DATA_BEGIN);
    }

    // 页头字段偏移
    static constexpr uint32_t OFFSET_PAGE_ID = 0;
    static constexpr uint32_t OFFSET_PREV_PAGE_ID = 4;
    static constexpr uint32_t OFFSET_NEXT_PAGE_ID = 8;
    static constexpr uint32_t OFFSET_NUM_SLOTS = 12;
    static constexpr uint32_t OFFSET_DATA_BEGIN = 16;  // uint16_t
};

}  // namespace hdb
