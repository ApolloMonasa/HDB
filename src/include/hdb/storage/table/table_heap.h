#pragma once

#include <mutex>

#include "hdb/common/config.h"
#include "hdb/common/rid.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"

namespace hdb {

class BufferPoolManager;
class TableIterator;

/**
 * @file table_heap.h
 * @brief Lab 2 Task 3：表堆——把"一张表"摊平成页面链表。
 *
 * TableHeap = 首页 page_id + 一组对页面的操作。表可以无限增长：
 * 当前页插不下就新建一页挂到链尾。上层（执行器）只跟 TableHeap
 * 打交道，完全不知道页面链的存在。
 *
 *   page1 ⇄ page2 ⇄ page3 ⇄ …（双向链，next/prev 在 TablePage 头里）
 *
 * 并发提示：HDB 骨架对表级并发从简（一把大互斥锁保护"找尾页 /
 * 扩链"的临界区）；真正的行级并发在 Lab5 由锁管理器接管。
 */
class TableHeap {
  public:
    /**
     * 打开已存在的表。
     * @param first_page_id 首页页号（Catalog 里记着）
     * @param schema        表模式（Catalog 里记着）
     */
    TableHeap(BufferPoolManager *bpm, page_id_t first_page_id, Schema schema);

    /** 新建空表：分配首页并 Init（给定实现） */
    static TableHeap *Create(BufferPoolManager *bpm, Schema schema);

    ~TableHeap();

    /**
     * TODO(Lab2): 插入一条记录（从尾页开始尝试，满了新建页链接）。
     * @param[out] rid 写回记录地址
     * @return false 理论上仅在磁盘分配失败时发生
     */
    bool InsertTuple(const Tuple &tuple, RID *rid);

    /** TODO(Lab2): 按 rid 读记录（跨页寻址：page_id → 帧 → 槽） */
    bool GetTuple(const RID &rid, Tuple *tuple) const;

    /** TODO(Lab2): 墓碑删除（定位页 + TablePage::MarkDelete） */
    bool MarkDelete(const RID &rid);

    /**
     * TODO(Lab2): 更新：先试原位更新（UpdateTupleInPlace），
     * 放不下则 MarkDelete + 重新 InsertTuple（新 rid 写回 *new_rid）。
     */
    bool UpdateTuple(const Tuple &tuple, const RID &rid, RID *new_rid);

    /** 表的首个有效记录迭代器（给定签名；实现在 table_iterator 上） */
    TableIterator Begin() const;

    /** 尾后迭代器 */
    TableIterator End() const;

    page_id_t GetFirstPageId() const { return first_page_id_; }

    /** 迭代器跨页行走需要经缓冲池取页（B+ 树同款暴露方式） */
    BufferPoolManager *GetBufferPoolManager() const { return bpm_; }

    /** 测试辅助：表内所有页的页号（按链表序） */
    std::vector<page_id_t> GetAllPageIds() const;

  private:
    BufferPoolManager *bpm_;
    page_id_t first_page_id_;
    Schema schema_;
    mutable std::mutex table_lock_;  // 保护"扩页"临界区
};

}  // namespace hdb
