/**
 * test/buffer/buffer_pool_manager_test.cpp —— Lab 1 Task 3 判卷用例
 */
#include <cstring>
#include <thread>
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/config.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/page/page.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_bpm_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}
}  // namespace

/** 新建页 → 写数据 → Unpin → Flush → 重新 Fetch 数据还在 */
TEST(BufferPoolManagerTest, NewPageWriteFlushRefetch) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);
        EXPECT_EQ(page->GetPageId(), pid);
        EXPECT_EQ(page->GetPinCount(), 1);

        char marker[] = "HDB::buffer_pool_manager_test";
        std::memcpy(page->GetData(), marker, sizeof(marker));

        EXPECT_TRUE(bpm.UnpinPage(pid, true));
        EXPECT_TRUE(bpm.FlushPage(pid));

        Page *fetched = bpm.FetchPage(pid);
        ASSERT_NE(fetched, nullptr);
        EXPECT_EQ(0, std::memcmp(fetched->GetData(), marker, sizeof(marker)));
        EXPECT_TRUE(bpm.UnpinPage(pid, false));
    }
    std::remove(file.c_str());
}

/** 池容量 3，建 5 页：第 4 页必然驱逐第 1 页（无人 pin 的帧按 LRU-K 逐出） */
TEST(BufferPoolManagerTest, EvictUnpinnedPage) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pids[5];
        for (int i = 0; i < 5; i++) {
            Page *page = bpm.NewPage(&pids[i]);
            ASSERT_NE(page, nullptr);
            std::memcpy(page->GetData(), &i, sizeof(i));
            EXPECT_TRUE(bpm.UnpinPage(pids[i], true));  // 用完就还
        }

        // 池只有 3 帧：最先使用的 0、1 号页应已被驱逐出池
        EXPECT_FALSE(bpm.InPool(pids[0]));
        EXPECT_FALSE(bpm.InPool(pids[1]));
        EXPECT_TRUE(bpm.InPool(pids[2]));

        // 驱逐的是脏页 → 重新 Fetch 时数据必须还在（已写回磁盘）
        Page *p0 = bpm.FetchPage(pids[0]);
        ASSERT_NE(p0, nullptr);
        int marker;
        std::memcpy(&marker, p0->GetData(), sizeof(marker));
        EXPECT_EQ(marker, 0);
        EXPECT_TRUE(bpm.UnpinPage(pids[0], false));
    }
    std::remove(file.c_str());
}

/** 全池被 pin 时无法驱逐：NewPage 返回 nullptr */
TEST(BufferPoolManagerTest, AllPinnedNoEviction) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pid;
        for (int i = 0; i < 3; i++) {
            ASSERT_NE(bpm.NewPage(&pid), nullptr);  // 全部 pin 住不还
        }
        EXPECT_EQ(bpm.NewPage(&pid), nullptr);  // 无帧可用
        // 释放一个后又能新建
        EXPECT_TRUE(bpm.UnpinPage(pid, false));
        EXPECT_NE(bpm.NewPage(&pid), nullptr);
    }
    std::remove(file.c_str());
}

/** DeletePage：被 pin 的页删不掉；unpin 后能删，且磁盘页号被释放 */
TEST(BufferPoolManagerTest, DeletePage) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(5, &dm);

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);

        EXPECT_EQ(bpm.DeletePage(pid), nullptr);  // pin 计数 1：删除失败
        EXPECT_TRUE(bpm.UnpinPage(pid, false));
        EXPECT_NE(bpm.DeletePage(pid), nullptr);  // 现在删除成功
        EXPECT_FALSE(bpm.InPool(pid));
    }
    std::remove(file.c_str());
}

/** FlushAllPages：关库路径 */
TEST(BufferPoolManagerTest, FlushAllPages) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        page_id_t pids[4];
        for (int i = 0; i < 4; i++) {
            Page *page = bpm.NewPage(&pids[i]);
            ASSERT_NE(page, nullptr);
            std::memcpy(page->GetData(), &i, sizeof(i));
            EXPECT_TRUE(bpm.UnpinPage(pids[i], true));
        }
        bpm.FlushAllPages();
        // 即使页仍驻留内存，也全部落盘（FlushPage 语义对脏标记的清理）
        for (int i = 0; i < 4; i++) {
            Page *p = bpm.FetchPage(pids[i]);
            ASSERT_NE(p, nullptr);
            int marker;
            std::memcpy(&marker, p->GetData(), sizeof(marker));
            EXPECT_EQ(marker, i);
            EXPECT_TRUE(bpm.UnpinPage(pids[i], false));
        }
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
