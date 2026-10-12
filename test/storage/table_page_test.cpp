/**
 * test/storage/table_page_test.cpp —— Lab 2 Task 2 判卷用例
 * @note 依赖 Lab1 的 BufferPoolManager 已实现（页面经缓冲池获取）
 */
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/config.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/page/page.h"
#include "hdb/storage/page/table_page.h"
#include "hdb/storage/table/column.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_tablepage_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema MakeSchema() { return Schema({Column("id", TypeId::INTEGER), Column("name", TypeId::VARCHAR, 24)}); }

Tuple MakeTuple(int id, const std::string &name) {
    return Tuple({ValueFactory::CreateInteger(id), ValueFactory::CreateVarchar(name)}, MakeSchema());
}
}  // namespace

TEST(TablePageTest, InitAndPageChainFields) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);
        auto *tp = reinterpret_cast<TablePage *>(page->GetData());
        tp->Init(pid, INVALID_PAGE_ID);

        EXPECT_EQ(tp->GetTablePageId(), pid);
        EXPECT_EQ(tp->GetPrevPageId(), INVALID_PAGE_ID);
        EXPECT_EQ(tp->GetNextPageId(), INVALID_PAGE_ID);
        EXPECT_EQ(tp->GetNumSlots(), 0U);
        EXPECT_GT(tp->GetFreeSpaceSize(), PAGE_SIZE - 64);

        bpm.UnpinPage(pid, true);
    }
    std::remove(file.c_str());
}

TEST(TablePageTest, InsertGetDelete) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);
        Schema schema = MakeSchema();

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);
        auto *tp = reinterpret_cast<TablePage *>(page->GetData());
        tp->Init(pid, INVALID_PAGE_ID);

        // 插三条
        RID rid;
        ASSERT_TRUE(tp->InsertTuple(MakeTuple(1, "alice"), &rid));
        ASSERT_TRUE(tp->InsertTuple(MakeTuple(2, "bob"), &rid));
        ASSERT_TRUE(tp->InsertTuple(MakeTuple(3, "carol"), &rid));
        EXPECT_EQ(tp->GetNumSlots(), 3U);

        // 取回校验
        Tuple out;
        ASSERT_TRUE(tp->GetTuple(RID(pid, 0), &out, schema));
        EXPECT_EQ(out.GetValue(schema, 0).GetAsInteger(), 1);
        EXPECT_EQ(out.GetValue(schema, 1).GetAsVarchar(), "alice");

        // 删除第 2 条（墓碑）
        ASSERT_TRUE(tp->MarkDelete(RID(pid, 1)));
        Tuple deleted;
        EXPECT_FALSE(tp->GetTuple(RID(pid, 1), &deleted, schema));
        EXPECT_TRUE(tp->IsDeleted(1));
        // 存活的记录不受影响
        ASSERT_TRUE(tp->GetTuple(RID(pid, 2), &out, schema));
        EXPECT_EQ(out.GetValue(schema, 1).GetAsVarchar(), "carol");

        bpm.UnpinPage(pid, true);
    }
    std::remove(file.c_str());
}

TEST(TablePageTest, PageFullReturnsFalse) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);
        Schema schema = MakeSchema();

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);
        auto *tp = reinterpret_cast<TablePage *>(page->GetData());
        tp->Init(pid, INVALID_PAGE_ID);

        // 每条 ~4+28=32B，4096B 页插到满
        RID rid;
        int inserted = 0;
        while (tp->InsertTuple(MakeTuple(inserted, "row"), &rid)) {
            inserted++;
        }
        // 至少插进去 100 条（页头 20B + 每条 32B 数据 + 4B 槽）
        EXPECT_GT(inserted, 100);
        bpm.UnpinPage(pid, true);
    }
    std::remove(file.c_str());
}

TEST(TablePageTest, IterateLiveTuples) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(3, &dm);
        Schema schema = MakeSchema();

        page_id_t pid;
        Page *page = bpm.NewPage(&pid);
        ASSERT_NE(page, nullptr);
        auto *tp = reinterpret_cast<TablePage *>(page->GetData());
        tp->Init(pid, INVALID_PAGE_ID);

        RID rid;
        for (int i = 0; i < 6; i++) {
            ASSERT_TRUE(tp->InsertTuple(MakeTuple(i, "t" + std::to_string(i)), &rid));
        }
        tp->MarkDelete(RID(pid, 2));
        tp->MarkDelete(RID(pid, 4));

        // 从头迭代：应只看到 0,1,3,5
        std::vector<int> seen;
        RID cur = RID(pid, -1);
        while (true) {
            cur = tp->GetNextTupleRid(cur);
            if (cur.GetPageId() == INVALID_PAGE_ID) {
                break;
            }
            Tuple t;
            ASSERT_TRUE(tp->GetTuple(cur, &t, schema));
            seen.push_back(t.GetValue(schema, 0).GetAsInteger());
        }
        EXPECT_EQ(seen, (std::vector<int>{0, 1, 3, 5}));

        bpm.UnpinPage(pid, true);
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
