#include "hdb/storage/page/page_guard.h"

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/exception.h"
#include "hdb/storage/page/page.h"

namespace hdb {

// 实现说明：Lab1 Task 4 的填空位。当前全部抛 NotImplementedException。
// 移动语义是本文件的教学重点：一遍一遍问自己
// "被移动后的旧守卫析构时会发生什么？"

BasicPageGuard::BasicPageGuard(BufferPoolManager *bpm, Page *page) : bpm_(bpm), page_(page) {}

BasicPageGuard::~BasicPageGuard() {
    // TODO(Lab1): 析构调用 Drop()（析构不抛异常）
}

BasicPageGuard::BasicPageGuard(BasicPageGuard &&other) noexcept {
    // TODO(Lab1): 移动资源（bpm_/page_/is_dirty_）并把 other 置空。
    // 桩刻意留空：测试会因 pin 未正确归还而失败，从而暴露未实现。
    (void)other;
}

BasicPageGuard &BasicPageGuard::operator=(BasicPageGuard &&other) noexcept {
    // TODO(Lab1): 先 Drop 自己，再接收 other 的资源，最后把 other 置空；注意自赋值安全。
    (void)other;
    return *this;
}

void BasicPageGuard::Drop() { throw NotImplementedException("Lab1: implement BasicPageGuard::Drop"); }

ReadPageGuard::ReadPageGuard(BufferPoolManager *bpm, Page *page) : BasicPageGuard(bpm, page) {}

ReadPageGuard::~ReadPageGuard() {
    // TODO(Lab1): 析构调用 Drop()（析构不抛异常）
}

ReadPageGuard::ReadPageGuard(ReadPageGuard &&other) noexcept {
    // TODO(Lab1): 基类移动 + 转移 locked_ 状态
    (void)other;
}

ReadPageGuard &ReadPageGuard::operator=(ReadPageGuard &&other) noexcept {
    // TODO(Lab1): 基类移动赋值 + 转移 locked_ 状态
    (void)other;
    return *this;
}

void ReadPageGuard::Drop() { throw NotImplementedException("Lab1: implement ReadPageGuard::Drop"); }

WritePageGuard::WritePageGuard(BufferPoolManager *bpm, Page *page) : BasicPageGuard(bpm, page) {}

WritePageGuard::~WritePageGuard() {
    // TODO(Lab1): 析构调用 Drop()（析构不抛异常）
}

WritePageGuard::WritePageGuard(WritePageGuard &&other) noexcept {
    // TODO(Lab1): 基类移动 + 转移 locked_ 状态
    (void)other;
}

WritePageGuard &WritePageGuard::operator=(WritePageGuard &&other) noexcept {
    // TODO(Lab1): 基类移动赋值 + 转移 locked_ 状态
    (void)other;
    return *this;
}

void WritePageGuard::Drop() { throw NotImplementedException("Lab1: implement WritePageGuard::Drop"); }

// ---------------------------------------------------------------------------
// 缓冲池的守卫工厂（框架给定）：取页 + 上锁一步完成
// ---------------------------------------------------------------------------
ReadPageGuard BufferPoolManager::ReadPageGuardFor(page_id_t page_id) {
    return ReadPageGuard(this, FetchPage(page_id));
}

WritePageGuard BufferPoolManager::WritePageGuardFor(page_id_t page_id) {
    return WritePageGuard(this, FetchPage(page_id));
}

}  // namespace hdb
