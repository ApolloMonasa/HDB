/**
 * test/storage/b_plus_tree_sequential_scale_test.cpp —— Lab 3 规模与迭代器判卷用例
 */
#include <string>

#include "gtest/gtest.h"
#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/storage/index/b_plus_tree.h"
#include "hdb/storage/index/index_iterator.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/table/column.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
std::string MakeTempFile() {
    static int seq = 0;
    return "/tmp/hdb_bpt_seq_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema KeySchema() { return Schema({Column("k", TypeId::INTEGER)}); }

IndexKey K(int64_t v) {
    IndexKey key;
    key.SerializeFrom(ValueFactory::CreateBigint(v));
    return key;
}
}  // namespace

/** 大批量顺序键（最坏情况的分裂模式）+ 全量范围扫描有序性 */
TEST(BPlusTreeSequentialScaleTest, SequentialKeysAndScan) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(64, &dm);
        auto *tree = new BPlusTree(&bpm, TypeId::BIGINT, "pk", KeySchema());
        tree->Init();

        constexpr int64_t N = 5000;
        for (int64_t i = 0; i < N; i++) {
            ASSERT_TRUE(tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i))));
        }

        // 全表扫描：键必须严格递增且一个不少
        int64_t expect = 0;
        for (auto it = tree->Begin(); it != tree->End(); ++it) {
            EXPECT_EQ((*it).GetSlotId(), expect);
            expect++;
        }
        EXPECT_EQ(expect, N);
        delete tree;
    }
    std::remove(file.c_str());
}

/** 范围扫描：Begin(key) 从中间起跳 */
TEST(BPlusTreeSequentialScaleTest, RangeScanFromMiddle) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(64, &dm);
        auto *tree = new BPlusTree(&bpm, TypeId::BIGINT, "pk", KeySchema());
        tree->Init();

        for (int64_t i = 0; i < 1000; i++) {
            tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i)));
        }

        // 从 500 开始扫：应看到 [500, 1000)
        int64_t expect = 500;
        for (auto it = tree->Begin(K(500)); it != tree->End(); ++it) {
            EXPECT_EQ((*it).GetSlotId(), expect);
            expect++;
        }
        EXPECT_EQ(expect, 1000);

        // 从不存在的键起跳：定位到第一个 >= 它的位置
        expect = 0;
        for (auto it = tree->Begin(K(-100)); it != tree->End(); ++it) {
            expect++;
        }
        EXPECT_EQ(expect, 1000);
        delete tree;
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
