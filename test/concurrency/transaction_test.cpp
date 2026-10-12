/**
 * test/concurrency/transaction_test.cpp —— Lab 5 判卷用例（事务管理器）
 */
#include <thread>

#include <thread>

#include "gtest/gtest.h"
#include "hdb/concurrency/transaction_manager.h"
#include "hdb/hdb_instance.h"

namespace hdb::test {

TEST(TransactionTest, BeginAssignsMonotonicIds) {
    TransactionManager tm;
    Transaction *t1 = tm.Begin();
    Transaction *t2 = tm.Begin();
    EXPECT_EQ(t1->GetState(), TxnState::GROWING);
    EXPECT_GT(t2->GetTxnId(), t1->GetTxnId());
}

TEST(TransactionTest, CommitReleasesLocks) {
    TransactionManager tm;
    LockManager &lm = tm.GetLockManager();

    Transaction *t1 = tm.Begin();
    lm.LockShared(t1, "a");
    lm.LockExclusive(t1, "b");

    tm.Commit(t1);
    EXPECT_EQ(t1->GetState(), TxnState::COMMITTED);

    // 锁应已全部释放：另一事务可直接拿 b 的 X 锁（不阻塞）
    Transaction *t2 = tm.Begin();
    lm.LockExclusive(t2, "b");
    lm.Unlock(t2, "b");
    tm.Commit(t2);
}

TEST(TransactionTest, AbortReleasesLocks) {
    TransactionManager tm;
    LockManager &lm = tm.GetLockManager();

    Transaction *t1 = tm.Begin();
    lm.LockExclusive(t1, "a");
    tm.Abort(t1);
    EXPECT_EQ(t1->GetState(), TxnState::ABORTED);

    Transaction *t2 = tm.Begin();
    lm.LockExclusive(t2, "a");  // 不阻塞说明 t1 的锁被正确释放
    lm.Unlock(t2, "a");
    tm.Commit(t2);
}

/** Lab5 Task3：语句级事务联动——并发写同表在锁保护下不丢数据 */
TEST(TransactionSqlTest, ConcurrentInsertsWithTxn) {
    const char *DB = "/tmp/hdb_txn_sql_test.db";
    std::remove(DB);
    {
        HdbInstance db(DB);
        db.SetTxnEnabled(true);  // Lab5 Task3 打开
        db.ExecuteSql("CREATE TABLE t (id INT PRIMARY KEY, v INT)");

        constexpr int kThreads = 4, kPerThread = 50;
        std::vector<std::thread> ths;
        for (int t = 0; t < kThreads; t++) {
            ths.emplace_back([&, t] {
                for (int i = 0; i < kPerThread; i++) {
                    db.ExecuteSql("INSERT INTO t VALUES (" + std::to_string(t * 1000 + i) + ", 0)");
                }
            });
        }
        for (auto &th : ths) {
            th.join();
        }
        auto r = db.ExecuteSql("SELECT COUNT(id) FROM t");
        ASSERT_TRUE(r.ok) << r.error;
        ASSERT_EQ(r.rows.size(), 1U);
        EXPECT_EQ(r.rows[0].GetValue(r.row_schema, 0).GetAsBigint(),
                  static_cast<int64_t>(kThreads * kPerThread));
    }
    std::remove(DB);
}

}  // namespace hdb::test
