/**
 * hdb-bench —— 吞吐基准（对标 BusTub leaderboard 的入门版）
 *
 * 用法：
 *   ./build/tools/benchmark/hdb-bench <bpm|btree|sql> [规模参数]
 *
 *   bpm   缓冲池：多线程混合访问（热页 zipfian + 全表扫描），输出页访问/s
 *   btree B+ 树：顺序插入 + 随机插入 + 随机点查，各阶段输出 ops/s
 *   sql   SQL 层：INSERT / 点查 SELECT / 范围扫描，输出语句/s
 *
 * 学生完成对应 Lab 后跑分对比参考实现——性能不是评分项，
 * 但"为什么我的比参考慢 10 倍"是最好的内核学习问题。
 */
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "hdb/buffer/buffer_pool_manager.h"
#include "hdb/common/exception.h"
#include "hdb/hdb_instance.h"
#include "hdb/storage/index/b_plus_tree.h"
#include "hdb/storage/disk/disk_manager.h"
#include "hdb/storage/table/column.h"
#include "hdb/type/value_factory.h"

using namespace hdb;
using Clock = std::chrono::steady_clock;

namespace {

double Secs(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}

void BenchBpm(size_t pages, int threads, int seconds) {
    const char *db = "/tmp/hdb_bench_bpm.db";
    std::remove(db);
    DiskManager dm(db);
    BufferPoolManager bpm(512, &dm);

    // 预建数据页
    std::vector<page_id_t> pids;
    for (size_t i = 0; i < pages; i++) {
        page_id_t pid;
        Page *p = bpm.NewPage(&pid);
        std::memset(p->GetData(), static_cast<int>(i), PAGE_SIZE);
        bpm.UnpinPage(pid, true);
        pids.push_back(pid);
    }

    std::atomic<bool> stop{false};
    std::atomic<uint64_t> hits{0}, misses{0};
    std::vector<std::thread> ths;
    for (int t = 0; t < threads; t++) {
        ths.emplace_back([&, t] {
            std::mt19937 rng(t);
            std::uniform_int_distribution<size_t> zip(0, pids.size() / 8);  // 热区
            std::uniform_int_distribution<size_t> uni(0, pids.size() - 1);
            size_t scan = 0;
            while (!stop.load()) {
                page_id_t pid;
                if (scan++ % 8 != 0) {
                    pid = pids[zip(rng)];   // 87.5% 热区访问
                } else {
                    pid = pids[uni(rng)];   // 12.5% 全表扫描污染
                }
                Page *p = bpm.FetchPage(pid);
                if (p != nullptr) {
                    p->GetData()[0] ^= 1;
                    bpm.UnpinPage(pid, true);
                    hits.fetch_add(1);
                } else {
                    misses.fetch_add(1);
                }
            }
        });
    }
    auto t0 = Clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    for (auto &th : ths) {
        th.join();
    }
    double sec = Secs(t0, Clock::now());
    std::printf("[bpm] pages=%zu threads=%d time=%.1fs\n", pages, threads, sec);
    std::printf("      throughput: %.0f fetch/s (hit=%llu miss=%llu)\n",
                hits.load() / sec, (unsigned long long)hits.load(), (unsigned long long)misses.load());
    std::remove(db);
}

void BenchBtree(size_t n) {
    const char *db = "/tmp/hdb_bench_btree.db";
    std::remove(db);
    DiskManager dm(db);
    BufferPoolManager bpm(256, &dm);
    Schema key_schema({Column("k", TypeId::INTEGER)});
    BPlusTree tree(&bpm, TypeId::BIGINT, "bench", key_schema);
    tree.Init();

    auto key = [](int64_t v) {
        IndexKey k;
        k.SerializeFrom(ValueFactory::CreateBigint(v));
        return k;
    };

    // 顺序插入
    auto t0 = Clock::now();
    for (int64_t i = 0; i < static_cast<int64_t>(n); i++) {
        tree.InsertEntry(key(i), RID(1, static_cast<slot_id_t>(i)));
    }
    auto t1 = Clock::now();
    // 随机插入
    std::vector<int64_t> shuffled(n);
    for (int64_t i = 0; i < static_cast<int64_t>(n); i++) {
        shuffled[i] = n + i;
    }
    std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937(42));
    for (int64_t v : shuffled) {
        tree.InsertEntry(key(v), RID(2, 0));
    }
    auto t2 = Clock::now();
    // 随机点查
    std::mt19937 rng(7);
    std::uniform_int_distribution<int64_t> uni(0, static_cast<int64_t>(n) * 2 - 1);
    uint64_t found = 0;
    const int lookups = 100000;
    for (int i = 0; i < lookups; i++) {
        std::vector<RID> r;
        tree.ScanKey(key(uni(rng)), &r);
        found += r.size();
    }
    auto t3 = Clock::now();

    std::printf("[btree] n=%zu\n", n);
    std::printf("  sequential insert: %8.0f ops/s\n", n / Secs(t0, t1));
    std::printf("  random insert:     %8.0f ops/s\n", n / Secs(t1, t2));
    std::printf("  random lookup:     %8.0f ops/s (hit rate %.1f%%)\n", lookups / Secs(t2, t3),
                100.0 * found / lookups);
    std::remove(db);
}

void BenchSql(size_t n) {
    const char *db = "/tmp/hdb_bench_sql.db";
    std::remove(db);
    HdbInstance inst(db);
    inst.ExecuteSql("CREATE TABLE t (id INT PRIMARY KEY, a INT, b VARCHAR(8))");

    auto t0 = Clock::now();
    for (size_t i = 0; i < n; i++) {
        inst.ExecuteSql("INSERT INTO t VALUES (" + std::to_string(i) + ", " + std::to_string(i % 97) +
                        ", 'x')");
    }
    auto t1 = Clock::now();
    const int point = 20000;
    std::mt19937 rng(3);
    std::uniform_int_distribution<size_t> uni(0, n - 1);
    for (int i = 0; i < point; i++) {
        inst.ExecuteSql("SELECT a FROM t WHERE id = " + std::to_string(uni(rng)));
    }
    auto t2 = Clock::now();
    const int scans = 50;
    for (int i = 0; i < scans; i++) {
        inst.ExecuteSql("SELECT COUNT(id) FROM t WHERE a > 48");
    }
    auto t3 = Clock::now();

    std::printf("[sql] rows=%zu\n", n);
    std::printf("  INSERT:          %8.0f stmt/s\n", n / Secs(t0, t1));
    std::printf("  point SELECT:    %8.0f stmt/s\n", point / Secs(t1, t2));
    std::printf("  aggregate scan:  %8.0f stmt/s\n", scans / Secs(t2, t3));
    std::remove(db);
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::printf("usage: %s <bpm|btree|sql> [scale]\n", argv[0]);
        return 1;
    }
    std::string mode = argv[1];
    size_t scale = argc > 2 ? std::stoull(argv[2]) : 0;
    try {
        if (mode == "bpm") {
            BenchBpm(scale ? scale : 4096, 8, 5);
        } else if (mode == "btree") {
            BenchBtree(scale ? scale : 100000);
        } else if (mode == "sql") {
            BenchSql(scale ? scale : 20000);
        } else {
            std::printf("unknown mode: %s\n", mode.c_str());
            return 1;
        }
    } catch (const NotImplementedException &e) {
        std::printf("benchmark needs the corresponding lab implemented:\n  %s\n", e.what());
        return 2;
    }
    return 0;
}
