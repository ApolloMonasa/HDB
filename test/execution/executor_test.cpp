#include <algorithm>
/**
 * test/execution/executor_test.cpp —— Lab 4（执行部分）判卷用例
 *
 * 走完整流水线（Lexer→Parser→Planner→Optimizer→Executor），
 * 依赖你在 Lab0-3 的成果 + 本 Lab 的执行器实现。
 * 桩状态下全部失败是预期行为。
 */
#include <string>

#include "gtest/gtest.h"
#include "hdb/hdb_instance.h"

namespace hdb::test {

namespace {
const char *DB = "/tmp/hdb_executor_test.db";

/** 每个用例独立的全新实例 */
struct Fixture {
    std::unique_ptr<HdbInstance> db;
    Fixture() {
        std::remove(DB);
        db = std::make_unique<HdbInstance>(DB);
        Exec("CREATE TABLE student (id INT PRIMARY KEY, name VARCHAR(16), gpa FLOAT)");
    }
    ~Fixture() { std::remove(DB); }

    SqlResult Exec(const std::string &sql) { return db->ExecuteSql(sql); }

    std::vector<std::string> Rows(const std::string &sql) {
        auto r = Exec(sql);
        EXPECT_TRUE(r.ok) << r.error;
        return r.RenderRows();
    }
};
}  // namespace

TEST(ExecutorTest, InsertAndSeqScan) {
    Fixture fx;
    EXPECT_TRUE(fx.Exec("INSERT INTO student VALUES (1, 'alice', 3.5)").ok);
    EXPECT_TRUE(fx.Exec("INSERT INTO student VALUES (2, 'bob', 3.0)").ok);
    EXPECT_TRUE(fx.Exec("INSERT INTO student VALUES (3, 'carol', 3.8)").ok);

    auto rows = fx.Rows("SELECT id, name FROM student");
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_EQ(rows[0], "1|alice");
    EXPECT_EQ(rows[2], "3|carol");
}

TEST(ExecutorTest, SeqScanFilterProjection) {
    Fixture fx;
    fx.Exec("INSERT INTO student VALUES (1, 'a', 2.0)");
    fx.Exec("INSERT INTO student VALUES (2, 'b', 3.9)");
    fx.Exec("INSERT INTO student VALUES (3, 'c', 3.1)");

    auto rows = fx.Rows("SELECT name FROM student WHERE gpa > 3.0");
    ASSERT_EQ(rows.size(), 2U);
    EXPECT_EQ(rows[0], "b");
    EXPECT_EQ(rows[1], "c");
}

TEST(ExecutorTest, DeleteAndUpdate) {
    Fixture fx;
    fx.Exec("INSERT INTO student VALUES (1, 'a', 2.0)");
    fx.Exec("INSERT INTO student VALUES (2, 'b', 3.9)");
    fx.Exec("INSERT INTO student VALUES (3, 'c', 3.1)");

    EXPECT_EQ(fx.Exec("DELETE FROM student WHERE id = 2").affected, 1);
    auto rows = fx.Rows("SELECT id FROM student");
    ASSERT_EQ(rows.size(), 2U);

    EXPECT_EQ(fx.Exec("UPDATE student SET gpa = 4.0 WHERE id = 3").affected, 1);
    rows = fx.Rows("SELECT gpa FROM student WHERE id = 3");
    ASSERT_EQ(rows.size(), 1U);
    EXPECT_EQ(rows[0], "4");
}

TEST(ExecutorTest, DuplicatePrimaryKeyRejected) {
    Fixture fx;
    EXPECT_TRUE(fx.Exec("INSERT INTO student VALUES (1, 'a', 2.0)").ok);
    auto r = fx.Exec("INSERT INTO student VALUES (1, 'again', 3.0)");
    EXPECT_FALSE(r.ok);  // 主键冲突
}

TEST(ExecutorTest, Join) {
    Fixture fx;
    fx.Exec("CREATE TABLE course (cid INT PRIMARY KEY, student_id INT, title VARCHAR(20))");
    fx.Exec("INSERT INTO student VALUES (1, 'alice', 3.5)");
    fx.Exec("INSERT INTO student VALUES (2, 'bob', 3.0)");
    fx.Exec("INSERT INTO course VALUES (10, 1, 'db')");
    fx.Exec("INSERT INTO course VALUES (11, 1, 'os')");
    fx.Exec("INSERT INTO course VALUES (12, 2, 'db')");

    auto rows = fx.Rows(
        "SELECT student.name, course.title FROM student JOIN course ON student.id = course.student_id");
    ASSERT_EQ(rows.size(), 3U);
    // 顺序依赖实现，只断言集合
    EXPECT_NE(std::find(rows.begin(), rows.end(), std::string("alice|db")), rows.end());
    EXPECT_NE(std::find(rows.begin(), rows.end(), std::string("alice|os")), rows.end());
    EXPECT_NE(std::find(rows.begin(), rows.end(), std::string("bob|db")), rows.end());
}

TEST(ExecutorTest, AggregateGroupBy) {
    Fixture fx;
    fx.Exec("CREATE TABLE t (a INT, b INT)");
    fx.Exec("INSERT INTO t VALUES (1, 10)");
    fx.Exec("INSERT INTO t VALUES (1, 20)");
    fx.Exec("INSERT INTO t VALUES (2, 30)");

    auto rows = fx.Rows("SELECT a, COUNT(b) FROM t GROUP BY a");
    ASSERT_EQ(rows.size(), 2U);
    EXPECT_NE(std::find(rows.begin(), rows.end(), std::string("1|2")), rows.end());
    EXPECT_NE(std::find(rows.begin(), rows.end(), std::string("2|1")), rows.end());
}

TEST(ExecutorTest, SortLimitTopN) {
    Fixture fx;
    fx.Exec("CREATE TABLE t (a INT)");
    for (int i = 0; i < 20; i++) {
        fx.Exec("INSERT INTO t VALUES (" + std::to_string(i) + ")");
    }
    auto rows = fx.Rows("SELECT a FROM t ORDER BY a DESC LIMIT 3");
    ASSERT_EQ(rows.size(), 3U);
    EXPECT_EQ(rows[0], "19");
    EXPECT_EQ(rows[1], "18");
    EXPECT_EQ(rows[2], "17");
}

}  // namespace hdb::test
