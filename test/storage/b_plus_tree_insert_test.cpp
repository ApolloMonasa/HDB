/**
 * test/storage/b_plus_tree_insert_test.cpp —— Lab 3 插入路径判卷用例
 * @note 依赖 Lab1 的 BufferPoolManager（树页经缓冲池访问）
 */
#include <algorithm>
#include <random>
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
    return "/tmp/hdb_bpt_ins_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema KeySchema() { return Schema({Column("k", TypeId::INTEGER)}); }

IndexKey K(int64_t v) {
    IndexKey key;
    key.SerializeFrom(ValueFactory::CreateBigint(v));
    return key;
}

/** 建一棵整型键的树（每个测试独立建） */
BPlusTree *MakeTree(BufferPoolManager *bpm) {
    auto *tree = new BPlusTree(bpm, TypeId::BIGINT, "pk", KeySchema());
    tree->Init();
    return tree;
}
}  // namespace

TEST(BPlusTreeInsertTest, InsertAndGetValue) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        for (int64_t i = 1; i <= 50; i++) {
            EXPECT_TRUE(tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i)))) << "i=" << i;
        }
        for (int64_t i = 1; i <= 50; i++) {
            std::vector<RID> result;
            tree->ScanKey(K(i), &result);
            ASSERT_EQ(result.size(), 1U) << "key=" << i;
            EXPECT_EQ(result[0].GetSlotId(), i);
        }
        // 不存在的键
        std::vector<RID> none;
        tree->ScanKey(K(999), &none);
        EXPECT_TRUE(none.empty());
        delete tree;
    }
    std::remove(file.c_str());
}

TEST(BPlusTreeInsertTest, DuplicateKeyRejected) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        EXPECT_TRUE(tree->InsertEntry(K(1), RID(1, 1)));
        EXPECT_FALSE(tree->InsertEntry(K(1), RID(1, 2)));  // 唯一索引：重复键拒绝
        std::vector<RID> result;
        tree->ScanKey(K(1), &result);
        ASSERT_EQ(result.size(), 1U);
        EXPECT_EQ(result[0].GetSlotId(), 1);
        delete tree;
    }
    std::remove(file.c_str());
}

TEST(BPlusTreeInsertTest, ReverseOrderInsertSplits) {
    // 逆序插入同样驱动多次分裂，检验中间键上推与兄弟链接
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(32, &dm);
        auto *tree = MakeTree(&bpm);

        for (int64_t i = 200; i >= 1; i--) {
            EXPECT_TRUE(tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i))));
        }
        for (int64_t i = 1; i <= 200; i++) {
            std::vector<RID> result;
            tree->ScanKey(K(i), &result);
            ASSERT_EQ(result.size(), 1U);
            EXPECT_EQ(result[0].GetSlotId(), i);
        }
        delete tree;
    }
    std::remove(file.c_str());
}

TEST(BPlusTreeInsertTest, RandomOrderLargeScale) {
    // 2000 个键乱序插入：分裂路径的无死角检查
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(64, &dm);
        auto *tree = MakeTree(&bpm);

        std::vector<int64_t> keys;
        for (int64_t i = 0; i < 2000; i++) {
            keys.push_back(i);
        }
        std::mt19937 rng(42);  // 固定种子：可复现
        std::shuffle(keys.begin(), keys.end(), rng);

        for (int64_t k : keys) {
            ASSERT_TRUE(tree->InsertEntry(K(k), RID(2, static_cast<slot_id_t>(k))));
        }
        for (int64_t k = 0; k < 2000; k++) {
            std::vector<RID> result;
            tree->ScanKey(K(k), &result);
            ASSERT_EQ(result.size(), 1U) << "k=" << k;
            EXPECT_EQ(result[0].GetSlotId(), k);
        }
        delete tree;
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
