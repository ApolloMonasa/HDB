/**
 * test/primer/trie_test.cpp —— Lab 0 判卷用例（第一部分：纯 Trie 语义）
 *
 * 桩状态下这些用例会因 NotImplementedException 失败——这是预期行为；
 * 完成 src/include/hdb/primer/trie.h 中的实现后应全部转绿。
 */
#include "gtest/gtest.h"
#include "hdb/primer/trie.h"

namespace hdb::test {

TEST(TrieTest, InsertAndGet) {
    Trie trie;
    trie = trie.Put("apple", "1");
    trie = trie.Put("application", "2");
    trie = trie.Put("banana", "3");

    std::string out;
    EXPECT_TRUE(trie.Get("apple", &out));
    EXPECT_EQ(out, "1");
    EXPECT_TRUE(trie.Get("application", &out));
    EXPECT_EQ(out, "2");
    EXPECT_TRUE(trie.Get("banana", &out));
    EXPECT_EQ(out, "3");
}

TEST(TrieTest, GetMissingKey) {
    Trie trie;
    trie = trie.Put("apple", "1");

    std::string out;
    EXPECT_FALSE(trie.Get("app", &out));       // 只是前缀，不是完整 key
    EXPECT_FALSE(trie.Get("applepie", &out));  // 超出长度的 key
    EXPECT_FALSE(trie.Get("", &out));          // 空 key（除非真插入过空 key）
}

TEST(TrieTest, OverwriteValue) {
    Trie trie;
    trie = trie.Put("key", "old");
    trie = trie.Put("key", "new");
    std::string out;
    EXPECT_TRUE(trie.Get("key", &out));
    EXPECT_EQ(out, "new");
}

TEST(TrieTest, RemoveKey) {
    Trie trie;
    trie = trie.Put("apple", "1");
    trie = trie.Put("applesauce", "2");

    trie = trie.Remove("apple");
    std::string out;
    EXPECT_FALSE(trie.Get("apple", &out));
    EXPECT_TRUE(trie.Get("applesauce", &out));  // 邻居 key 不受影响
    EXPECT_EQ(out, "2");
}

TEST(TrieTest, CopyIsDeep) {
    Trie t1;
    t1 = t1.Put("abc", "1");
    Trie t2 = t1;  // 拷贝构造
    t2 = t2.Put("abcd", "2");

    std::string out;
    EXPECT_FALSE(t1.Get("abcd", &out));  // 在副本上加的值不应出现在原件里
    EXPECT_TRUE(t2.Get("abc", &out));    // 原有的值仍在
}

TEST(TrieTest, SnapshotIsStable) {
    // 写时复制的核心性质：拿到旧树后继续 Put，旧树读到的内容不变
    Trie t1;
    t1 = t1.Put("a", "1");
    Trie t2 = t1;
    t2 = t2.Put("a", "mutated");
    std::string out;
    EXPECT_TRUE(t1.Get("a", &out));
    EXPECT_EQ(out, "1");  // 旧树保持旧值
}

}  // namespace hdb::test
