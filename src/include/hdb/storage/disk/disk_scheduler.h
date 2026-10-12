#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <memory>
#include <optional>
#include <thread>

#include "hdb/common/channel.h"
#include "hdb/common/config.h"

namespace hdb {

class DiskManager;

/**
 * @file disk_scheduler.h
 * @brief Lab 1 Task 2：磁盘调度器——把同步的 DiskManager 变成异步服务。
 *
 * 上层（BufferPoolManager）发起一次磁盘读写后不想原地干等：
 * 请求被丢进队列，后台线程完成 I/O 后用 promise"回电"。
 * std::future::get() 于是成了"等这次 I/O 落地"的同步点。
 *
 * 结构图：
 *
 *   调用线程                              后台工作线程
 *   ─────────                              ─────────────
 *   req = {is_write, page_id, data,
 *          promise<bool>}
 *        │ Push(req)
 *        ▼
 *   └─► Channel<std::optional<DiskRequest>> ──► Pop(req) ──► DiskManager
 *        │                                        │ 成功: callback.set_value(true)
 *        └─ future = req.callback.get_future()    └ 失败: callback.set_value(false)
 *            ...做别的事...              future.get() == true ?
 *
 * 为什么用 std::optional 包装？Kill 通道时投递一个 nullopt 作为"退场暗号"。
 */
struct DiskRequest {
    bool is_write_{false};       ///< true 写 / false 读
    page_id_t page_id_{INVALID_PAGE_ID};
    char *data_{nullptr};        ///< 读写缓冲（读是输出、写是输入）
    std::promise<bool> callback_{};  ///< 完成回电：true 成功 / false 失败
};

class DiskScheduler {
  public:
    explicit DiskScheduler(DiskManager *disk_manager);

    /**
     * TODO(Lab1): 析构必须优雅：
     *   1. Kill 通道（投递 nullopt 或调用 Channel::Kill）；
     *   2. join 后台线程。
     * 忘掉这一步，程序退出时会挂在未 join 的线程上。
     */
    ~DiskScheduler();

    DiskScheduler(const DiskScheduler &) = delete;
    DiskScheduler &operator=(const DiskScheduler &) = delete;

    /** TODO(Lab1): 把请求投进队列（移动语义，promise 不能拷贝） */
    void Schedule(DiskRequest request);

    /**
     * TODO(Lab1): 后台线程主循环：Pop 到请求就调 DiskManager 完成读写，
     * 无论成败都要 callback_.set_value(...)，否则调用方 future.get() 永久挂起。
     * Pop 返回 false（通道已 Kill 且排空）时退出循环。
     */
    void StartWorkerInBackground();

  private:
    DiskManager *disk_manager_{nullptr};  ///< 不持有所有权
    std::optional<DiskRequest> request_{};
    Channel<std::optional<DiskRequest>> request_channel_{};
    std::thread background_thread_;
};

}  // namespace hdb
