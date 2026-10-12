/**
 * test/concurrency/mvcc_test.cpp —— Lab 6（选做）判卷用例
 */
#include <algorithm>
#include <thread>

#include "gtest/gtest.h"
#include "hdb/concurrency/mvcc.h"
#include "hdb/storage/table/column.h"
#include "hdb/storage/table/schema.h"
#include "hdb/storage/table/tuple.h"
#include "hdb/type/value_factory.h"

namespace hdb::test {

namespace {
Schema MakeSchema() { return Schema({Column("v", TypeId::INTEGER)}); }

Tuple MakeTuple(int v) { return Tuple({ValueFactory::CreateInteger(v)}, MakeSchema()); }

std::vector<VersionRecord> Chain(std::vector<std::pair<uint64_t, int>> ts_values) {
    std::vector<VersionRecord> versions;
    for (auto it = ts_values.rbegin(); it != ts_values.rend(); ++it) {  // 新→旧
        versions.push_back(VersionRecord{it->first, false, MakeTuple(it->second)});
    }
    return versions;
}
}  // namespace

TEST(MvccTest, TimestampsMonotonic) {
    MvccManager m;
    uint64_t r1 = m.BeginReadTimestamp();
    uint64_t c1 = m.CommitTimestamp();
    uint64_t r2 = m.BeginReadTimestamp();
    EXPECT_EQ(r1, 0U);
    EXPECT_EQ(c1, 1U);
    EXPECT_EQ(r2, 1U);  // 新事务的眼镜包含上一次提交
    EXPECT_LT(r2, m.CommitTimestamp());
}

TEST(MvccTest, SnapshotReadStable) {
    // v=1 已提交 → T1 戴上眼镜(read_ts=1) → 写者提交 v=2 →
    // T1 重建仍见 v=1；v=2 对 T1 不可见
    MvccManager m;
    m.CommitTimestamp();                       // ts=1: v=1 已提交
    uint64_t read_ts = m.BeginReadTimestamp(); // T1 快照 = 1
    m.CommitTimestamp();                       // ts=2: 写者提交 v=2

    auto versions = Chain({{1, 1}, {2, 2}});   // 新→旧 [{ts2,v2},{ts1,v1}]
    Tuple out;
    ASSERT_TRUE(m.ReconstructTuple(read_ts, versions, &out));
    EXPECT_EQ(out.GetValue(MakeSchema(), 0).GetAsInteger(), 1);  // 稳定：仍是旧值
    EXPECT_FALSE(m.IsVisible(read_ts, 2, false));  // 快照之后提交：不可见

    // 新事务能看到 v=2
    uint64_t fresh = m.BeginReadTimestamp();
    ASSERT_TRUE(m.ReconstructTuple(fresh, versions, &out));
    EXPECT_EQ(out.GetValue(MakeSchema(), 0).GetAsInteger(), 2);
}

TEST(MvccTest, VisibilityBoundary) {
    MvccManager m;
    m.CommitTimestamp();  // last = 1
    uint64_t read_ts = m.BeginReadTimestamp();  // = 1

    // commit_ts == read_ts 恰好可见（<= 边界）
    EXPECT_TRUE(m.IsVisible(read_ts, 1, false));
    EXPECT_FALSE(m.IsVisible(read_ts, 2, false));
    Tuple out;
    ASSERT_TRUE(m.ReconstructTuple(read_ts, Chain({{2, 20}, {1, 10}}), &out));
    EXPECT_EQ(out.GetValue(MakeSchema(), 0).GetAsInteger(), 10);  // 取 <= 1 的最新版
}

TEST(MvccTest, DeletedVersionInvisible) {
    MvccManager m;
    m.CommitTimestamp();
    uint64_t read_ts = m.BeginReadTimestamp();
    // 快照之后被删除（墓碑 ts=2）
    std::vector<VersionRecord> versions{
        VersionRecord{2, true, Tuple{}},
        VersionRecord{1, false, MakeTuple(7)},
    };
    Tuple out;
    ASSERT_TRUE(m.ReconstructTuple(read_ts, versions, &out));  // 删除未生效：仍可见
    // 快照覆盖删除时间点的读者
    uint64_t later = m.CommitTimestamp();  // = 2
    EXPECT_FALSE(m.ReconstructTuple(later, versions, &out));  // 删除已生效
    EXPECT_FALSE(m.IsVisible(later, 2, true));
}

TEST(MvccTest, ConcurrentCommitSerialization) {
    // 提交时间戳的分配必须串行（无重复）
    MvccManager m;
    std::vector<uint64_t> results(16);
    std::vector<std::thread> ths;
    for (int i = 0; i < 16; i++) {
        ths.emplace_back([&, i] { results[i] = m.CommitTimestamp(); });
    }
    for (auto &t : ths) {
        t.join();
    }
    std::sort(results.begin(), results.end());
    for (size_t i = 1; i < results.size(); i++) {
        EXPECT_NE(results[i], results[i - 1]);  // 无重复
    }
    EXPECT_EQ(results.back(), m.LastCommitTs());
}

}  // namespace hdb::test
