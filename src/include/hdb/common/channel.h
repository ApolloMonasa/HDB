#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace hdb {

/**
 * @brief 线程安全的阻塞通道（框架给定）——Go 语言 channel 的 C++ 翻版。
 *
 * Lab 1 的 DiskScheduler 用它把磁盘请求递给后台工作线程：
 *   - Schedule 线程（可能很多个）在队尾塞请求；
 *   - 唯一的后台线程从队头取请求执行；
 *   - 队列空时后台线程挂起等待，不空转烧 CPU。
 *
 * 析构前调用 Kill() 唤醒所有等待者（后台线程借此退出）。
 */
template <typename T>
class Channel {
  public:
    Channel() = default;
    ~Channel() { Kill(); }

    /// 放入一个元素并唤醒一个等待的消费者
    void Push(T item) {
        {
            std::lock_guard<std::mutex> lk(m_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    /**
     * 取出一个元素；队列空则阻塞。
     * @return false 表示通道已被 Kill（消费者应当退出循环）
     */
    bool Pop(T &item) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this] { return killed_ || !queue_.empty(); });
        if (killed_ && queue_.empty()) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /// 唤醒所有等待者并令后续 Pop 在排空后返回 false
    void Kill() {
        {
            std::lock_guard<std::mutex> lk(m_);
            killed_ = true;
        }
        cv_.notify_all();
    }

  private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool killed_ = false;
};

}  // namespace hdb
