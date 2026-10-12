/**
 * test/storage/disk_manager_test.cpp
 *
 * DiskManager 是框架给定组件，本测试开局就应全绿——
 * 它同时演示了"页 = 4KB 字节块"的最基本事实，建议动手 Lab1 前先读一遍。
 */
#include <cstring>
#include <string>

#include "gtest/gtest.h"
#include "hdb/common/config.h"
#include "hdb/common/exception.h"
#include "hdb/storage/disk/disk_manager.h"

namespace {

using hdb::PAGE_SIZE;
using hdb::page_id_t;

std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_disk_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

/// 在缓冲区每个页的起始位置写入页号，读回时校验
void FillPageMarker(char *data, page_id_t page_id) {
    std::memset(data, 0, PAGE_SIZE);
    std::memcpy(data, &page_id, sizeof(page_id));
}

}  // namespace

namespace hdb::test {

/** 新建文件：页号从 1 开始分配，写后读回一致 */
TEST(DiskManagerTest, BasicWriteRead) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        EXPECT_EQ(dm.GetNumPages(), 1);  // 只有元数据页

        char write_buf[PAGE_SIZE], read_buf[PAGE_SIZE];
        for (page_id_t pid = 1; pid <= 5; pid++) {
            ASSERT_EQ(dm.IncreaseNumPages(), pid);
            FillPageMarker(write_buf, pid);
            dm.WritePage(pid, write_buf);

            std::memset(read_buf, 0, PAGE_SIZE);
            dm.ReadPage(pid, read_buf);
            EXPECT_EQ(0, std::memcmp(write_buf, read_buf, PAGE_SIZE));
        }
        EXPECT_EQ(dm.GetNumPages(), 6);
    }  // 析构刷元数据
    std::remove(file.c_str());
}

/** 重新打开文件：页数与数据都保持（持久性） */
TEST(DiskManagerTest, PersistenceAcrossReopen) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        page_id_t pid = dm.IncreaseNumPages();
        char buf[PAGE_SIZE];
        FillPageMarker(buf, pid);
        dm.WritePage(pid, buf);
    }
    {
        DiskManager dm(file);
        EXPECT_EQ(dm.GetNumPages(), 2);  // 元数据页 + 1 个数据页
        char buf[PAGE_SIZE];
        dm.ReadPage(1, buf);
        page_id_t marker;
        std::memcpy(&marker, buf, sizeof(marker));
        EXPECT_EQ(marker, 1);
    }
    std::remove(file.c_str());
}

/** 非法页号（0 号元数据页 / 越界 / 负数）必须抛 IOException */
TEST(DiskManagerTest, InvalidPageIdThrows) {
    std::string file = MakeTempFile();
    DiskManager dm(file);
    char buf[PAGE_SIZE] = {0};

    EXPECT_THROW(dm.ReadPage(0, buf), IOException);      // 元数据页不可直接读
    EXPECT_THROW(dm.WritePage(0, buf), IOException);
    EXPECT_THROW(dm.ReadPage(99, buf), IOException);     // 越界
    EXPECT_THROW(dm.ReadPage(-1, buf), IOException);
    EXPECT_THROW(dm.DeallocatePage(42), IOException);
    std::remove(file.c_str());
}

/** 释放页后，新分配的页号优先复用被释放的页 */
TEST(DiskManagerTest, DeallocateAndReuse) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        dm.IncreaseNumPages();  // 页 1
        dm.IncreaseNumPages();  // 页 2
        dm.IncreaseNumPages();  // 页 3
        EXPECT_EQ(dm.GetNumPages(), 4);

        dm.DeallocatePage(2);  // 释放页 2
        // free list: [2]，下次分配复用 2 而不是扩展到页 4
        EXPECT_EQ(dm.IncreaseNumPages(), 2);
        EXPECT_EQ(dm.GetNumPages(), 4);
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
