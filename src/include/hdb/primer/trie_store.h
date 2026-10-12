#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "hdb/primer/trie.h"

namespace hdb {

/**
 * @brief Lab 0 第二部分：把"不可变 Trie"包装成可共享的键值存储。
 *
 * 不可变结构解决了读并发，但总得有人去"换根"——TrieStore 就是那个
 * 拿着写锁换根的角色。这个"共享不可变根 + 换根升级"的模型，
 * 与数据库里的 MVCC 快照、RCU、持久化数据结构一脉相承。
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │ 并发结构（两把锁，缺一不可）                                   │
 * │                                                              │
 * │   Get:  store_lock_.RLock ──► 拷走 root_ ──► RUnlock         │
 * │         之后在快照上无锁查找（root_ 指向的树永不改变）          │
 * │                                                              │
 * │   Put/Remove: write_lock_ 独占（同一时刻只有一个写者构造新树） │
 * │   完成后 store_lock_.WLock 换根                                │
 * └──────────────────────────────────────────────────────────────┘
 *
 * 想一想：如果只有一个 store_lock_（读写锁）行不行？
 * 行，但写者构造新树期间会阻塞所有读者——两级锁让"造树"无锁化。
 */
class TrieStore {
  public:
    TrieStore() = default;

    /** TODO(Lab0): 取一份当前树的快照并返回。（实现在 trie.cpp） */
    Trie Get();

    /**
     * TODO(Lab0): 等价于 root_ = std::move(root_).Put(key, value) 的并发安全版。（实现在 trie.cpp）
     * 记得先用 root_.reset() 释放旧树（每一步都在写锁内）。
     */
    void Put(std::string key, std::string value);

    /** TODO(Lab0): 同 Put，只是换成了 Remove。（实现在 trie.cpp） */
    void Remove(const std::string &key);

  private:
    /** 换根需要独占；注意 root_ 的类型与 Trie 里不同（去掉 const 才能换） */
    std::shared_ptr<Trie::TrieNode> root_{nullptr};

    std::shared_mutex store_lock_;  ///< 保护 root_ 的换根
    std::mutex write_lock_;         ///< 串行化写者（造新树期间不必拦读者）
};

}  // namespace hdb
