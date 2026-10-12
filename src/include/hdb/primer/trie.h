#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace hdb {

class Trie;

/**
 * @file trie.h
 * @brief Lab 0：并发前缀树（Trie）——你要实现的第一份内核代码。
 *
 * 为什么数据库课要从一棵字典树开始？
 *   1. 它是"不可变数据结构 + 写时复制 + 快照读"的最小教学样本——
 *      这正是 MVCC（Lab6）与多版本存储的思想内核；
 *   2. 它逼你把 C++ 的移动语义、智能指针、深拷贝写正确——
 *      这些武器在后面五个 Lab 里天天用。
 *
 * ┌─────────────────────────────────────────────────────────────────┐
 * │ 设计约定（先读懂再动手）                                          │
 * │                                                                 │
     * │ * Trie 是**不可变**的：Put/Remove 不修改旧树，而是返回一棵新树：  │
     * │   先 Clone 出独占副本，再在副本上改写路径（值经 shared_ptr 共享）。 │
 * │ * 因此并发 Get 无需加锁：读到的 root_ 指向的树永远不会被改掉。    │
 * │ * TrieNode 只表示"路径经过"；TrieNodeWithValue 才带值。           │
 * │ * 值统一为 std::string，用 shared_ptr 挂在叶子上                  │
 * │   （为什么是 shared_ptr 不是 unique_ptr？想想快照读谁还握着它）。 │
 * └─────────────────────────────────────────────────────────────────┘
 *
 * 建议实现顺序：Clone() → Put() → Get() → Remove()。
 * 对应测试：test/primer/trie_test.cpp；你的实现写在 src/primer/trie.cpp。
 */
class Trie {
  public:
    /** 普通节点：只承担"路径经过"职责 */
    class TrieNode {
      public:
        TrieNode() = default;

        /**
         * 构造已有子节点表的节点（拷贝一份 map；map 里存的是 unique_ptr，
         * 因此拷贝构造要求你实现 Clone 而不是依赖编译器默认版本）
         */
        explicit TrieNode(std::unordered_map<char, std::unique_ptr<TrieNode>> children)
            : children_(std::move(children)) {}

        virtual ~TrieNode() = default;

        /** 是否是某个 key 的终点（即使终点也可能没有值，见 Remove 的剪枝） */
        bool IsEndNode() const { return is_end_; }
        void SetEndNode(bool is_end) { is_end_ = is_end; }

        std::unordered_map<char, std::unique_ptr<TrieNode>> &GetChildren() { return children_; }
        const std::unordered_map<char, std::unique_ptr<TrieNode>> &GetChildren() const {
            return children_;
        }

        /** 深拷贝整个子树（Put 的写时复制路径上，"要改的节点"先克隆一份） */
        virtual std::unique_ptr<TrieNode> Clone() const {
            auto node = std::make_unique<TrieNode>(std::unordered_map<char, std::unique_ptr<TrieNode>>());
            for (const auto &[ch, child] : children_) {
                node->children_[ch] = child->Clone();
            }
            node->is_end_ = is_end_;
            return node;
        }

      protected:
        std::unordered_map<char, std::unique_ptr<TrieNode>> children_;
        bool is_end_{false};
    };

    /** 带值的终点节点 */
    class TrieNodeWithValue : public TrieNode {
      public:
        TrieNodeWithValue() = default;

        explicit TrieNodeWithValue(std::shared_ptr<std::string> value) : value_(std::move(value)) {
            this->SetEndNode(true);
        }

        explicit TrieNodeWithValue(std::unordered_map<char, std::unique_ptr<TrieNode>> children,
                                   std::shared_ptr<std::string> value)
            : TrieNode(std::move(children)), value_(std::move(value)) {
            this->SetEndNode(true);
        }

        /** 克隆时连值一起复制（shared_ptr 拷贝，快照与新版共享同一份值对象） */
        std::unique_ptr<TrieNode> Clone() const override {
            auto node = std::make_unique<TrieNodeWithValue>(value_);
            for (const auto &[ch, child] : children_) {
                node->children_[ch] = child->Clone();
            }
            return node;
        }

        bool IsEndValue() const { return value_ != nullptr; }
        std::shared_ptr<std::string> GetValue() const { return value_; }
        void SetValue(std::shared_ptr<std::string> value) { value_ = std::move(value); }

      private:
        std::shared_ptr<std::string> value_;
    };

    // -----------------------------------------------------------------------
    // 你的实现区（以下四个函数的函数体当前都抛 NotImplementedException）
    // -----------------------------------------------------------------------

    Trie() = default;

    /** 拷贝构造：深拷贝 root_ 指向的整棵树（新旧树各自独立演化）。TODO(Lab0)（实现在 trie.cpp） */
    Trie(const Trie &other);

    Trie &operator=(const Trie &) = delete;   // 拷贝赋值禁用（返回值语义 + std::move 足够）
    Trie(Trie &&) = default;                  // 移动构造：搬 root_ 即可，编译器默认版本正确
    Trie &operator=(Trie &&) = default;

    /**
     * TODO(Lab0): 插入 (key, value)，返回**新的** Trie，原 Trie 保持不变。（实现在 trie.cpp）
     * 提示：沿 key 逐字符向下，每个要改动的节点 Clone 一份拼进新路径，
     *       没碰到的子树直接用 std::move 转移 unique_ptr（旧树反正被放弃）。
     * @note  按设计约定，调用方拿返回值后旧 Trie 不应再使用，
     *        TrieStore 里你会看到 std::move(trie).Put(...) 的用法。
     */
    Trie Put(const std::string &key, std::string value) const;

    /** TODO(Lab0): 查找 key。命中（is_end 且带值）则写入 *value 并返回 true。（实现在 trie.cpp） */
    bool Get(const std::string &key, std::string *value) const;

    /** TODO(Lab0): 删除 key，返回新 Trie；若删除后路径上不再有带值后代，应剪枝。（实现在 trie.cpp） */
    Trie Remove(const std::string &key) const;

    /** 空树判断（测试与 TrieStore 使用） */
    bool Empty() const { return root_ == nullptr; }

  private:
    /** 根节点：shared_ptr<const>——所有人共享、没人能改，这是无锁读的前提 */
    std::shared_ptr<const TrieNode> root_{nullptr};

    friend class TrieStore;  // 换根/取快照需要直接操纵 root_
};

}  // namespace hdb
