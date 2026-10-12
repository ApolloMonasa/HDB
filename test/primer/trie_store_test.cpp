/**
 * test/primer/trie_store_test.cpp —— Lab 0 判卷用例（第二部分：TrieStore 并发）
 */
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "hdb/primer/trie.h"
#include "hdb/primer/trie_store.h"

namespace hdb::test {

TEST(TrieStoreTest, BasicPutGet) {
    TrieStore store;
    store.Put("hello", "world");
    store.Put("hell", "fire");

    auto trie = store.Get();
    std::string out;
    EXPECT_TRUE(trie.Get("hello", &out));
    EXPECT_EQ(out, "world");
    EXPECT_TRUE(trie.Get("hell", &out));
    EXPECT_EQ(out, "fire");
}

TEST(TrieStoreTest, SnapshotIsolation) {
    // 快照隔离：Get 拿到的快照不受之后的写入影响
    TrieStore store;
    store.Put("k", "v1");
    auto snapshot = store.Get();
    store.Put("k", "v2");
    store.Put("new", "x");

    std::string out;
    EXPECT_TRUE(snapshot.Get("k", &out));
    EXPECT_EQ(out, "v1");
    EXPECT_FALSE(snapshot.Get("new", &out));
}

TEST(TrieStoreTest, ConcurrentWrites) {
    TrieStore store;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50;

    std::vector<std::thread> writers;
    for (int t = 0; t < kThreads; t++) {
        writers.emplace_back([&store, t] {
            for (int i = 0; i < kPerThread; i++) {
                store.Put("key_" + std::to_string(t) + "_" + std::to_string(i),
                          std::to_string(t * 10000 + i));
            }
        });
    }
    for (auto &w : writers) {
        w.join();
    }

    auto trie = store.Get();
    std::string out;
    for (int t = 0; t < kThreads; t++) {
        for (int i = 0; i < kPerThread; i++) {
            ASSERT_TRUE(trie.Get("key_" + std::to_string(t) + "_" + std::to_string(i), &out));
            EXPECT_EQ(out, std::to_string(t * 10000 + i));
        }
    }
}

TEST(TrieStoreTest, MixedReadersAndWriters) {
    // 读写混合不崩溃、不死锁；读者最终能看到全部写入
    TrieStore store;
    constexpr int kWriters = 4, kReaders = 4, kLoop = 100;

    std::vector<std::thread> ths;
    for (int w = 0; w < kWriters; w++) {
        ths.emplace_back([&store, w] {
            for (int i = 0; i < kLoop; i++) {
                store.Put("w" + std::to_string(w) + "_" + std::to_string(i), "v");
            }
        });
    }
    for (int r = 0; r < kReaders; r++) {
        ths.emplace_back([&store] {
            for (int i = 0; i < kLoop; i++) {
                auto t = store.Get();
                std::string out;
                (void)t.Get("w0_0", &out);  // 读不存在的 key 也行，只要不死锁
            }
        });
    }
    for (auto &t : ths) {
        t.join();
    }

    auto final_trie = store.Get();
    std::string out;
    EXPECT_TRUE(final_trie.Get("w3_99", &out));
}

}  // namespace hdb::test
