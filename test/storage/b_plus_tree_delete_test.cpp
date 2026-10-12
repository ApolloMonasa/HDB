/**
 * test/storage/b_plus_tree_delete_test.cpp —— Lab 3 删除路径判卷用例
 */
#include <algorithm>
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/storage/index/b_plus_tree.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/table/column.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_bpt_del_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema KeySchema() { return Schema({Column("k", TypeId::INTEGER)}); }

IndexKey K(int64_t v) {
    IndexKey key;
    key.SerializeFrom(ValueFactory::CreateBigint(v));
    return key;
}

BPlusTree *MakeTree(BufferPoolManager *bpm) {
    auto *tree = new BPlusTree(bpm, TypeId::BIGINT, "pk", KeySchema());
    tree->Init();
    return tree;
}
}  // namespace

TEST(BPlusTreeDeleteTest, DeleteSingles) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        for (int64_t i = 0; i < 100; i++) {
            tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i)));
        }
        // 删掉偶数键
        for (int64_t i = 0; i < 100; i += 2) {
            EXPECT_TRUE(tree->DeleteEntry(K(i), RID(1, static_cast<slot_id_t>(i))));
        }
        for (int64_t i = 0; i < 100; i++) {
            std::vector<RID> result;
            tree->ScanKey(K(i), &result);
            if (i % 2 == 0) {
                EXPECT_TRUE(result.empty()) << "deleted key=" << i << " still visible";
            } else {
                ASSERT_EQ(result.size(), 1U) << "key=" << i;
                EXPECT_EQ(result[0].GetSlotId(), i);
            }
        }
        // 重复删除返回 false
        EXPECT_FALSE(tree->DeleteEntry(K(0), RID(1, 0)));
        delete tree;
    }
    std::remove(file.c_str());
}

TEST(BPlusTreeDeleteTest, DeleteToEmpty) {
    // 删到一棵树变空：根收缩回 INVALID 的完整生命周期
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        constexpr int64_t N = 300;
        for (int64_t i = 0; i < N; i++) {
            tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i)));
        }
        EXPECT_FALSE(tree->IsEmpty());

        for (int64_t i = 0; i < N; i++) {
            EXPECT_TRUE(tree->DeleteEntry(K(i), RID(1, static_cast<slot_id_t>(i)))) << "i=" << i;
        }
        EXPECT_TRUE(tree->IsEmpty());

        // 空树之后还能继续插入
        EXPECT_TRUE(tree->InsertEntry(K(7), RID(1, 7)));
        std::vector<RID> result;
        tree->ScanKey(K(7), &result);
        EXPECT_EQ(result.size(), 1U);
        delete tree;
    }
    std::remove(file.c_str());
}

TEST(BPlusTreeDeleteTest, InterleavedInsertDelete) {
    // 边插边删：合并与分裂交替出现
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        for (int64_t round = 0; round < 5; round++) {
            for (int64_t i = 0; i < 100; i++) {
                tree->InsertEntry(K(round * 1000 + i), RID(1, static_cast<slot_id_t>(i)));
            }
            for (int64_t i = 0; i < 90; i++) {  // 留 10 个
                tree->DeleteEntry(K(round * 1000 + i), RID(1, static_cast<slot_id_t>(i)));
            }
        }
        for (int64_t round = 0; round < 5; round++) {
            for (int64_t i = 90; i < 100; i++) {
                std::vector<RID> result;
                tree->ScanKey(K(round * 1000 + i), &result);
                ASSERT_EQ(result.size(), 1U) << "round=" << round << " i=" << i;
            }
        }
        delete tree;
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
