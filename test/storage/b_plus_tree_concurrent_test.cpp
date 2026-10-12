/**
 * test/storage/b_plus_tree_concurrent_test.cpp —— Lab 3 蟹行并发判卷用例（必做）
 *
 * 这些测试同时通过 = 你的读写蟹行加锁正确：
 * 没有丢失更新、没有在持锁路径上死锁、树结构修改对并发读者原子可见。
 */
#include <atomic>
#include <string>
#include <thread>
#include <vector>

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
    return "/tmp/hdb_bpt_conc_" + std::to_string(getpid()) + "_" + std::to_string(seq++) + ".db";
}

Schema KeySchema() { return Schema({Column("k", TypeId::INTEGER)}); }

IndexKey K(int64_t v) {
    IndexKey key;
    key.SerializeFrom(ValueFactory::CreateBigint(v));
    return key;
}
}  // namespace

/** 每线程插自己的键区间，最后全部可见 */
TEST(BPlusTreeConcurrentTest, ConcurrentInsertDistinctRanges) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(64, &dm);
        auto *tree = new BPlusTree(&bpm, TypeId::BIGINT, "pk", KeySchema());
        tree->Init();

        constexpr int kThreads = 8, kPerThread = 300;
        std::vector<std::thread> ths;
        for (int t = 0; t < kThreads; t++) {
            ths.emplace_back([&, t] {
                for (int i = 0; i < kPerThread; i++) {
                    int64_t key = static_cast<int64_t>(t) * kPerThread + i;
                    ASSERT_TRUE(tree->InsertEntry(K(key), RID(1, static_cast<slot_id_t>(key))));
                }
            });
        }
        for (auto &th : ths) {
            th.join();
        }

        for (int64_t k = 0; k < kThreads * kPerThread; k++) {
            std::vector<RID> result;
            tree->ScanKey(K(k), &result);
            ASSERT_EQ(result.size(), 1U) << "k=" << k;
        }
        delete tree;
    }
    std::remove(file.c_str());
}

/** 并发读者与写者的隔离：写者插新键不干扰读者读旧键 */
TEST(BPlusTreeConcurrentTest, ReadersAndWriters) {
    std::string file = MakeTempFile();
    {
        DiskManager dm(file);
        BufferPoolManager bpm(64, &dm);
        auto *tree = new BPlusTree(&bpm, TypeId::BIGINT, "pk", KeySchema());
        tree->Init();

        // 预插旧键
        for (int64_t i = 0; i < 500; i++) {
            tree->InsertEntry(K(i), RID(1, static_cast<slot_id_t>(i)));
        }

        std::atomic<bool> stop{false};
        std::atomic<int> errors{0};

        std::vector<std::thread> ths;
        for (int r = 0; r < 4; r++) {
            ths.emplace_back([&] {
                while (!stop.load()) {
                    for (int64_t i = 0; i < 500; i++) {
                        std::vector<RID> result;
                        tree->ScanKey(K(i), &result);
                        if (result.size() != 1 || result[0].GetSlotId() != i) {
                            errors++;
                        }
                    }
                }
            });
        }
        for (int w = 0; w < 2; w++) {
            ths.emplace_back([&, w] {
                for (int64_t i = 0; i < 2000; i++) {
                    tree->InsertEntry(K(100000 + w * 10000 + i), RID(2, static_cast<slot_id_t>(i)));
                }
            });
        }
        // 写者线程结束后停止读者
        for (int i = 4; i < 6; i++) {
            ths[i].join();
        }
        stop.store(true);
        for (int i = 0; i < 4; i++) {
            ths[i].join();
        }
        EXPECT_EQ(errors.load(), 0);
        delete tree;
    }
    std::remove(file.c_str());
}

}  // namespace hdb::test
