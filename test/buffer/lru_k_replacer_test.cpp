#include <thread>
/**
 * test/buffer/lru_k_replacer_test.cpp —— Lab 1 Task 1 判卷用例
 *
 * 逐条核对头文件顶部"驱逐规则"框图里的每一条。
 */
#include "gtest/gtest.h"
#include "hdb/buffer/lru_k_replacer.h"

namespace hdb::test {

TEST(LRUKReplacerTest, SampleTest) {
    // BusTub 经典样例：K = 2
    LRUKReplacer lru_k(7, 2);

    // 访问 1 一次，访问 2 两次（达到 K 次），访问 3 一次
    lru_k.RecordAccess(1);
    lru_k.RecordAccess(2);
    lru_k.RecordAccess(2);
    lru_k.RecordAccess(3);
    lru_k.SetEvictable(1, true);
    lru_k.SetEvictable(2, true);
    lru_k.SetEvictable(3, true);

    EXPECT_EQ(lru_k.Size(), 3U);  // 三个帧都可驱逐

    frame_id_t fid;
    // 不足 K 次的帧 k-distance = +∞，且 1 比 3 更早访问 → 先驱逐 1
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 1);

    // 现在 3（1 次）与 2（2 次）：3 的 k-distance 仍是 +∞ → 驱逐 3
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 3);

    // 只剩 2
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 2);
    EXPECT_EQ(lru_k.Size(), 0U);
    EXPECT_FALSE(lru_k.Evict(&fid));  // 空了
}

TEST(LRUKReplacerTest, CannotEvictUnevictable) {
    LRUKReplacer lru_k(3, 2);
    lru_k.RecordAccess(1);
    lru_k.RecordAccess(2);
    lru_k.SetEvictable(1, true);
    lru_k.SetEvictable(2, false);  // 2 正被使用

    EXPECT_EQ(lru_k.Size(), 1U);  // 只有 1 可驱逐
    frame_id_t fid;
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 1);
    EXPECT_FALSE(lru_k.Evict(&fid));  // 2 不可驱逐 → 无帧可逐
}

TEST(LRUKReplacerTest, KDistanceRules) {
    // 都有 K 次访问后：驱逐 backward k-distance 最大的帧
    //   backward k-distance = 当前时间 - 第 K 次最近访问的时间（越大越"久远"）
    LRUKReplacer lru_k(3, 2);
    // 时间线：1,2,3,1,2,3 —— 三帧都有 2 次访问
    for (frame_id_t f : {1, 2, 3, 1, 2, 3}) {
        lru_k.RecordAccess(f);
    }
    for (frame_id_t f : {1, 2, 3}) {
        lru_k.SetEvictable(f, true);
    }
    // 历史：1={t0,t3} 2={t1,t4} 3={t2,t5}
    // 第 K 次最近访问（front）：1 的 t0 最老 → 1 的 k-distance 最大 → 驱逐 1
    frame_id_t fid;
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 1);
    // 再访问 1（其历史刷新为 {t3,t6}）：front 排序 2 的 t1 最老 → 驱逐 2
    lru_k.RecordAccess(1);
    ASSERT_TRUE(lru_k.Evict(&fid));
    EXPECT_EQ(fid, 2);
}

TEST(LRUKReplacerTest, RemoveErasesHistory) {
    LRUKReplacer lru_k(4, 2);
    lru_k.RecordAccess(1);
    lru_k.RecordAccess(1);
    lru_k.SetEvictable(1, true);
    EXPECT_EQ(lru_k.Size(), 1U);

    lru_k.Remove(1);  // 页被删除：彻底移除
    EXPECT_EQ(lru_k.Size(), 0U);

    // 重新记录 access 后（默认不可驱逐）不应立即参与驱逐
    lru_k.RecordAccess(1);
    frame_id_t fid;
    EXPECT_FALSE(lru_k.Evict(&fid));
}

TEST(LRUKReplacerTest, InvalidFrameIgnored) {
    LRUKReplacer lru_k(2, 2);
    lru_k.RecordAccess(100);   // 越界帧号：忽略，不崩溃
    lru_k.RecordAccess(-1);
    lru_k.SetEvictable(100, true);
    EXPECT_EQ(lru_k.Size(), 0U);
}

TEST(LRUKReplacerTest, ConcurrentAccess) {
    // 粗粒度并发正确性（不追求性能，只要求不死锁不崩溃）
    LRUKReplacer lru_k(16, 2);
    std::vector<std::thread> ths;
    for (int t = 0; t < 4; t++) {
        ths.emplace_back([&lru_k] {
            for (int i = 0; i < 200; i++) {
                lru_k.RecordAccess(i % 16);
                if (i % 3 == 0) {
                    lru_k.SetEvictable(i % 16, true);
                }
                frame_id_t fid;
                if (lru_k.Evict(&fid)) {
                    lru_k.Remove(fid);
                }
            }
        });
    }
    for (auto &th : ths) {
        th.join();
    }
}

}  // namespace hdb::test
