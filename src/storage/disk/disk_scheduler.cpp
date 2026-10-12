#include "hdb/storage/disk/disk_scheduler.h"

#include "hdb/common/exception.h"
#include "hdb/storage/disk/disk_manager.h"

namespace hdb {

// 实现说明：以下函数体是 Lab1 的填空位，当前抛出 NotImplementedException。
// 读 docs/lab1-buffer-pool.md 的 Task 2，配合 test/storage/disk_scheduler_test.cpp。

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
    throw NotImplementedException("Lab1: implement DiskScheduler constructor (start worker thread)");
}

DiskScheduler::~DiskScheduler() {
    // TODO(Lab1): Kill 通道并 join 后台线程（析构不抛异常）
}

void DiskScheduler::Schedule(DiskRequest request) {
    (void)request;
    throw NotImplementedException("Lab1: implement DiskScheduler::Schedule");
}

void DiskScheduler::StartWorkerInBackground() {
    // (无需参数处理)
    throw NotImplementedException("Lab1: implement DiskScheduler::StartWorkerInBackground");
}

}  // namespace hdb
