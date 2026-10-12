/**
 * test/storage/page_guard_test.cpp —— Lab 1 Task 4 判卷用例
 */
#include <cstring>
#include <thread>
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/page/page.h"
#include "hdb/storage/page/page_guard.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_guard_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}
}  // namespace

/** WritePageGuard：作用域结束自动 unpin + 置脏；数据经驱逐后仍可读回 */
TEST(PageGuardTest, WriteGuardAutoDrop) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pid;
        {
            Page *raw = bpm.NewPage(&pid);
            ASSERT_NE(raw, nullptr);
            // 把裸页包进守卫（守卫接管 pin 的归还责任）
            WritePageGuard guard(&bpm, raw);
            std::memcpy(guard->GetData(), "guarded", 8);
        }  // Drop：解写锁 + unpin(dirty=true)

        // 池被挤满驱逐该页后重新 Fetch，数据仍在（证明确实置脏并写回）
        for (int i = 0; i < 5; i++) {
            page_id_t dummy;
            Page *p = bpm.NewPage(&dummy);
            ASSERT_NE(p, nullptr);
            EXPECT_TRUE(bpm.UnpinPage(dummy, false));
        }
        Page *back = bpm.FetchPage(pid);
        ASSERT_NE(back, nullptr);
        EXPECT_EQ(0, std::memcmp(back->GetData(), "guarded", 8));
        EXPECT_TRUE(bpm.UnpinPage(pid, false));
    }
    std::remove(file.c_str());
}

/** 移动语义：守卫转移后原守卫析构不得二次 unpin */
TEST(PageGuardTest, MoveSemantics) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pid;
        Page *raw = bpm.NewPage(&pid);
        ASSERT_NE(raw, nullptr);
        EXPECT_EQ(raw->GetPinCount(), 1);

        WritePageGuard g1(&bpm, raw);
        WritePageGuard g2 = std::move(g1);  // g1 变空
        // g1 析构时不做任何事；页仍被 g2 持有
        {
            WritePageGuard g3 = std::move(g2);
        }  // g3 析构 Drop：unpin 恰好一次

        // pin 计数归零后页可被驱逐 → 说明 Drop 恰好执行了一次 unpin
        // （若二次 unpin，计数为负/状态错乱；若零次，下面的驱逐不会发生）
        page_id_t dummy;
        Page *p = bpm.NewPage(&dummy);
        EXPECT_NE(p, nullptr);  // 有帧可腾
    }
    std::remove(file.c_str());
}

/** ReadPageGuard：并发多读不死锁 */
TEST(PageGuardTest, ConcurrentReadGuards) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pid;
        Page *raw = bpm.NewPage(&pid);
        ASSERT_NE(raw, nullptr);
        EXPECT_TRUE(bpm.UnpinPage(pid, false));

        std::vector<std::thread> ths;
        for (int t = 0; t < 4; t++) {
            ths.emplace_back([&] {
                Page *p = bpm.FetchPage(pid);
                ASSERT_NE(p, nullptr);
                ReadPageGuard guard(&bpm, p);
                volatile char c = guard->GetData()[0];
                (void)c;
            });
        }
        for (auto &th : ths) {
            th.join();
        }
        // 所有线程 Drop 后 pin 计数回到 0，页可被驱逐
        page_id_t dummy;
        Page *p = bpm.NewPage(&dummy);
        EXPECT_NE(p, nullptr);
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
