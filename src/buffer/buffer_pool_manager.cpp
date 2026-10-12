#include "hdb/buffer/buffer_pool_manager.h"

#include <cstring>

#include "hdb/common/exception.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/disk/disk_scheduler.h"
#include "hdb/storage/page/page.h"

namespace hdb {

// 实现说明：以下函数体是 Lab1 的填空位，当前抛出 NotImplementedException。
// docs/lab1-buffer-pool.md Task 3 给了驱逐路径的分步清单，照着走，
// test/buffer/buffer_pool_manager_test.cpp 会逐一验证。

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k)
    : pool_size_(pool_size), disk_manager_(disk_manager) {
    (void)replacer_k;
    throw NotImplementedException("Lab1: implement BufferPoolManager constructor (docs/lab1-buffer-pool.md)");
}

BufferPoolManager::~BufferPoolManager() {
    // TODO(Lab1): 释放 pages_ 数组（析构不抛异常；disk_scheduler_ 由 unique_ptr 自动退出）
}

Page *BufferPoolManager::FetchPage(page_id_t page_id) {
    (void)page_id;
    throw NotImplementedException("Lab1: implement BufferPoolManager::FetchPage");
}

Page *BufferPoolManager::NewPage(page_id_t *page_id) {
    (void)page_id;
    throw NotImplementedException("Lab1: implement BufferPoolManager::NewPage");
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    (void)page_id;
    (void)is_dirty;
    throw NotImplementedException("Lab1: implement BufferPoolManager::UnpinPage");
}

bool BufferPoolManager::FlushPage(page_id_t page_id) {
    (void)page_id;
    throw NotImplementedException("Lab1: implement BufferPoolManager::FlushPage");
}

Page *BufferPoolManager::DeletePage(page_id_t page_id) {
    (void)page_id;
    throw NotImplementedException("Lab1: implement BufferPoolManager::DeletePage");
}

void BufferPoolManager::FlushAllPages() {
    throw NotImplementedException("Lab1: implement FlushAllPages"); }

bool BufferPoolManager::FindFreeFrame(frame_id_t *frame_id) {
    (void)frame_id;
    throw NotImplementedException("Lab1: implement BufferPoolManager::FindFreeFrame");
}

}  // namespace hdb
