/**
 * test/table/table_heap_test.cpp —— Lab 2 Task 3/4 判卷用例
 */
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/table/column.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/table_heap.h"
#include "hdb/storage/table/table_iterator.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_heap_test_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema MakeSchema() { return Schema({Column("id", TypeId::INTEGER), Column("name", TypeId::VARCHAR, 40)}); }

Tuple MakeTuple(int id, const std::string &name) {
    return Tuple({ValueFactory::CreateInteger(id), ValueFactory::CreateVarchar(name)}, MakeSchema());
}
}  // namespace

/** 建表 + 插入 + 读回 */
TEST(TableHeapTest, CreateInsertGet) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        TableHeap *heap = TableHeap::Create(&bpm, MakeSchema());
        RID rid;
        ASSERT_TRUE(heap->InsertTuple(MakeTuple(1, "alice"), &rid));
        ASSERT_TRUE(heap->InsertTuple(MakeTuple(2, "bob"), &rid));

        Tuple out;
        ASSERT_TRUE(heap->GetTuple(rid, &out));
        EXPECT_EQ(out.GetValue(MakeSchema(), 1).GetAsVarchar(), "bob");
        delete heap;
    }
    std::remove(file.c_str());
}

/** 插满多页：页面链自动增长，迭代器跨页遍历 */
TEST(TableHeapTest, MultiPageAndIterate) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        TableHeap *heap = TableHeap::Create(&bpm, MakeSchema());
        Schema schema = MakeSchema();

        // 每条 ~48B；插 500 条必然跨多页
        RID rid;
        for (int i = 0; i < 500; i++) {
            ASSERT_TRUE(heap->InsertTuple(MakeTuple(i, "row_" + std::to_string(i)), &rid));
        }
        EXPECT_GT(heap->GetAllPageIds().size(), 1U);

        // 迭代器应按插入序遍历全部 500 条
        int expect = 0;
        for (auto it = heap->Begin(); it != heap->End(); ++it) {
            Tuple t = *it;
            EXPECT_EQ(t.GetValue(schema, 0).GetAsInteger(), expect);
            expect++;
        }
        EXPECT_EQ(expect, 500);
        delete heap;
    }
    std::remove(file.c_str());
}

/** 删除后不可见：迭代器跳过墓碑 */
TEST(TableHeapTest, MarkDeleteAndIterate) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        TableHeap *heap = TableHeap::Create(&bpm, MakeSchema());
        Schema schema = MakeSchema();

        std::vector<RID> rids;
        RID rid;
        for (int i = 0; i < 20; i++) {
            ASSERT_TRUE(heap->InsertTuple(MakeTuple(i, "t"), &rid));
            rids.push_back(rid);
        }
        // 删偶数条
        for (int i = 0; i < 20; i += 2) {
            ASSERT_TRUE(heap->MarkDelete(rids[i]));
        }
        int expect = 1;
        for (auto it = heap->Begin(); it != heap->End(); ++it) {
            EXPECT_EQ((*it).GetValue(schema, 0).GetAsInteger(), expect);
            expect += 2;
        }
        EXPECT_EQ(expect, 21);
        delete heap;
    }
    std::remove(file.c_str());
}

/** 更新：短记录原位更新成功 */
TEST(TableHeapTest, UpdateTuple) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(10, &dm);

        TableHeap *heap = TableHeap::Create(&bpm, MakeSchema());
        Schema schema = MakeSchema();

        RID rid, new_rid;
        ASSERT_TRUE(heap->InsertTuple(MakeTuple(1, "a-very-long-name"), &rid));
        ASSERT_TRUE(heap->UpdateTuple(MakeTuple(1, "short"), rid, &new_rid));

        Tuple out;
        ASSERT_TRUE(heap->GetTuple(new_rid, &out));
        EXPECT_EQ(out.GetValue(schema, 1).GetAsVarchar(), "short");
        delete heap;
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
