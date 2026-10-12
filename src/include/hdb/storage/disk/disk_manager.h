#pragma once

#include <atomic>
#include <cstdio>
#include <string>

#include "hdb/common/config.h"

namespace hdb {

/**
 * @brief 磁盘管理器（框架给定，无需修改）——整个内核里唯一碰文件 I/O 的地方。
 *
 * 职责刻意做小：把"页号"翻译成"文件偏移"，做最朴素的同步读写。
 * 异步调度、缓存、替换统统不归它管——那是 Lab1 你要实现的
 * DiskScheduler 与 BufferPoolManager 的事。
 *
 * 文件布局：
 *
 *   页 0（元数据页）        [num_pages(4B)][free_head(4B)][...]
 *   页 1 .. num_pages-1     数据页（由上层结构解释内容）
 *
 * 空闲页链表：页被释放（DeallocatePage）后头插进 free list，
 * 链表 next 指针存在该页自身的前 4 字节里；
 * 之后 IncreaseNumPages() 优先复用链表上的页号。
 */
class DiskManager {
  public:
    /**
     * 打开（不存在则创建）数据库文件并读入元数据。
     * @throw IOException 文件无法打开 / 元数据损坏
     */
    explicit DiskManager(const std::string &db_file);

    /** 关闭文件前把元数据刷回第 0 页 */
    ~DiskManager();

    DiskManager(const DiskManager &) = delete;
    DiskManager &operator=(const DiskManager &) = delete;

    /**
     * 读一个页。
     * @param page_id 合法数据页号（1 <= page_id < num_pages）
     * @param page_data 输出缓冲，至少 PAGE_SIZE 字节
     * @throw IOException page_id 越界或 fread 失败
     */
    void ReadPage(page_id_t page_id, char *page_data);

    /**
     * 写一个页（同步落盘）。
     * @throw IOException page_id 越界或 fwrite 失败
     */
    void WritePage(page_id_t page_id, const char *page_data);

    /**
     * 分配一个新页号：优先复用 free list，否则扩展文件。
     * @return 新分配的数据页号（从 1 开始）
     */
    page_id_t IncreaseNumPages();

    /** 释放一个页：头插进 free list 等待复用 */
    void DeallocatePage(page_id_t page_id);

    /// 文件中的总页数（含元数据页）
    int GetNumPages() const { return num_pages_; }

    /// 数据库文件路径（测试与调试用）
    const std::string &GetFileName() const { return file_name_; }

  private:
    /** 把 num_pages_/free_head_ 写到第 0 页并 fflush（析构与显式调用） */
    void WriteMetadata();
    void ReadMetadataOrFormat();

    FILE *db_io_{nullptr};
    std::string file_name_;
    int num_pages_{1};   // 至少包含元数据页
    int32_t free_head_{INVALID_PAGE_ID};
    std::atomic<bool> io_failed_{false};
};

}  // namespace hdb
