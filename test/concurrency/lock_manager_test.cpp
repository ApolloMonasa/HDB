/**
 * test/concurrency/lock_manager_test.cpp —— Lab 5 判卷用例（锁管理器）
 *
 * 多数用例用线程 + 标志位验证"阻塞确实发生了"——
 * 这是并发测试的标准姿势（光测函数返回值测不出互斥）。
 */
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "hdb/common/exception.h"
#include "hdb/concurrency/lock_manager.h"

namespace hdb::test {

/** S 锁多读并存：两个事务同时持 S 不阻塞 */
TEST(LockManagerTest, SharedLocksCompatible) {
    LockManager lm;
    Transaction t1(1), t2(2);
    lm.LockShared(&t1, "t");
    lm.LockShared(&t2, "t");  // 不应阻塞
    EXPECT_EQ(lm.GrantedCount("t"), 2U);
    lm.Unlock(&t1, "t");
    lm.Unlock(&t2, "t");
}

/** X 锁互斥：t1 持 X 时 t2 的 X 被阻塞，释放后获得 */
TEST(LockManagerTest, ExclusiveBlocksExclusive) {
    LockManager lm;
    Transaction t1(1), t2(2);
    lm.LockExclusive(&t1, "t");

    std::atomic<bool> t2_got{false};
    std::thread waiter([&] {
        lm.LockExclusive(&t2, "t");
        t2_got.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(t2_got.load());  // t1 还持锁，t2 必须还在等

    lm.Unlock(&t1, "t");
    waiter.join();
    EXPECT_TRUE(t2_got.load());
    EXPECT_EQ(lm.GrantedCount("t"), 1U);
    lm.Unlock(&t2, "t");
}

/** S 与 X 互斥 */
TEST(LockManagerTest, ExclusiveBlocksShared) {
    LockManager lm;
    Transaction t1(1), t2(2);
    lm.LockExclusive(&t1, "t");

    std::atomic<bool> t2_got{false};
    std::thread waiter([&] {
        lm.LockShared(&t2, "t");
        t2_got.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(t2_got.load());

    lm.Unlock(&t1, "t");
    waiter.join();
    EXPECT_TRUE(t2_got.load());
    lm.Unlock(&t2, "t");
}

/** FIFO 防饿死：t2 先排队，t1 释放后即便 t3 与持锁者兼容也要排在 t2 之后 */
TEST(LockManagerTest, QueueIsFair) {
    LockManager lm;
    Transaction t1(1), t2(2), t3(3);
    lm.LockShared(&t1, "t");
    lm.LockShared(&t2, "t");

    std::atomic<bool> t3_got{false};
    // t3 请求 X：与 t1/t2 的 S 冲突 → 排队
    std::thread x_requester([&] {
        lm.LockExclusive(&t3, "t");
        t3_got.store(true);
        lm.Unlock(&t3, "t");
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(t3_got.load());

    // t1 释放（t2 仍持 S）：t3 依然拿不到
    lm.Unlock(&t1, "t");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(t3_got.load());

    // t2 也释放：t3 拿到
    lm.Unlock(&t2, "t");
    x_requester.join();
    EXPECT_TRUE(t3_got.load());
}

/** 升级：S -> X 独占后，他人的 S 等待 */
TEST(LockManagerTest, UpgradeToExclusive) {
    LockManager lm;
    Transaction t1(1), t2(2);
    lm.LockShared(&t1, "t");
    lm.LockShared(&t2, "t");

    std::atomic<bool> upgraded{false};
    std::thread upgrader([&] {
        lm.LockUpgrade(&t1, "t");  // t1: S->X，需等 t2 释放
        upgraded.store(true);
        lm.Unlock(&t1, "t");
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(upgraded.load());

    lm.Unlock(&t2, "t");
    upgrader.join();
    EXPECT_TRUE(upgraded.load());
}

/** 2PL 违规：收缩期再加锁必须回滚（抛 LockAbortException） */
TEST(LockManagerTest, TwoPhaseViolationAborts) {
    LockManager lm;
    Transaction t1(1);
    lm.LockShared(&t1, "t");
    lm.Unlock(&t1, "t");           // 进入 SHRINKING
    EXPECT_EQ(t1.GetState(), TxnState::SHRINKING);
    EXPECT_THROW(lm.LockShared(&t1, "t"), LockAbortException);
    EXPECT_THROW(lm.LockExclusive(&t1, "t"), LockAbortException);
}

}  // namespace hdb::test
