/**
 * test/storage/disk_scheduler_test.cpp —— Lab 1 Task 2 判卷用例
 */
#include <cstring>
#include <future>
#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "hdb/common/config.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/disk/disk_scheduler.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_sched_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}
}  // namespace

TEST(DiskSchedulerTest, AsyncWriteThenRead) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        DiskScheduler scheduler(&dm);

        page_id_t pid = dm.IncreaseNumPages();

        char write_buf[PAGE_SIZE], read_buf[PAGE_SIZE];
        std::memset(write_buf, 0xAB, PAGE_SIZE);

        // 异步写：promise/future 是唯一的完成信号
        std::promise<bool> write_done;
        auto write_future = write_done.get_future();
        scheduler.Schedule(DiskRequest{true, pid, write_buf, std::move(write_done)});
        ASSERT_TRUE(write_future.get());

        std::memset(read_buf, 0, PAGE_SIZE);
        std::promise<bool> read_done;
        auto read_future = read_done.get_future();
        scheduler.Schedule(DiskRequest{false, pid, read_buf, std::move(read_done)});
        ASSERT_TRUE(read_future.get());

        EXPECT_EQ(0, std::memcmp(write_buf, read_buf, PAGE_SIZE));
    }  // 析构必须优雅退出后台线程（不挂起、不 terminate）
    std::remove(file.c_str());
}

TEST(DiskSchedulerTest, ManyConcurrentRequests) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        DiskScheduler scheduler(&dm);

        constexpr int kPages = 32;
        std::vector<page_id_t> pids;
        for (int i = 0; i < kPages; i++) {
            pids.push_back(dm.IncreaseNumPages());
        }

        // 先全部写入
        std::vector<std::future<bool>> futures;
        auto bufs = std::make_unique<char[][PAGE_SIZE]>(kPages);
        for (int i = 0; i < kPages; i++) {
            std::memset(bufs[i], static_cast<char>(i), PAGE_SIZE);
            std::promise<bool> done;
            futures.push_back(done.get_future());
            scheduler.Schedule(DiskRequest{true, pids[i], bufs[i], std::move(done)});
        }
        for (auto &f : futures) {
            ASSERT_TRUE(f.get());
        }

        // 再全部读回校验
        auto read_bufs = std::make_unique<char[][PAGE_SIZE]>(kPages);
        futures.clear();
        for (int i = 0; i < kPages; i++) {
            std::promise<bool> done;
            futures.push_back(done.get_future());
            scheduler.Schedule(DiskRequest{false, pids[i], read_bufs[i], std::move(done)});
        }
        for (int i = 0; i < kPages; i++) {
            ASSERT_TRUE(futures[i].get());
            EXPECT_EQ(0, std::memcmp(bufs[i], read_bufs[i], PAGE_SIZE));
        }
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
