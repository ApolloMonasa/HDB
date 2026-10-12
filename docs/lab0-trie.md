# Lab 0：C++ 热身 —— 并发前缀树（Trie）

> 对标 CMU 15-445 Project 0（2023/2024 形态）。不计入正式难度，但不完成它，Lab1 的智能指针与锁会寸步难行。

## 概述

在动手写数据库组件之前，补齐本实验需要的 C++ 武器：

- `std::unique_ptr` 的**移动语义**（move constructor / move assignment）
- **递归拷贝**一个拥有堆内存的树结构（写对析构与深拷贝）
- `std::shared_ptr` + 只读快照的并发模式（写时复制思路）
- `std::shared_mutex` 读写锁的基本用法

本 Lab 涉及的文件：

| 文件 | 说明 |
|---|---|
| `src/include/hdb/primer/trie.h` | `TrieNode` / `TrieNodeWithValue` / `Trie`（实现处，头文件内标记 TODO） |
| `src/include/hdb/primer/trie_store.h` | `TrieStore`：读写锁保护下的共享 Trie |
| `src/primer/trie.cpp` | 头文件中**非模板**部分的实现位 |
| `test/primer/trie_test.cpp` | 拷贝/移动语义、双重释放（ASAN）、Remove 剪枝 |
| `test/primer/trie_store_test.cpp` | 并发快照隔离：写不影响已读快照 |

## 开始之前

- 通读 `trie.h` 顶部的规则说明——`Trie` 是**不可变数据结构**，`Put`/`Remove` 返回新 Trie，旧 Trie 及其快照永不被改动；
- 跑一遍测试确认基线（预期全部失败，异常信息会指向本文件）：

```bash
cmake --build build --target trie_test trie_store_test --parallel $(nproc)
./build/test/trie_test
./build/test/trie_store_test
```

## 背景

前缀树（Trie）按字符路径存键。把它做成**不可变 + 写时复制**（copy-on-write）后，
"修改"只克隆从根到目标节点的一条路径，其余子树用 `std::move` 挪进新树——
读者手里的旧快照（`shared_ptr<const TrieNode>`）与写者构造的新树互不干扰。
这就是无锁读并发的基本套路，也是后续理解 MVCC 快照读（Lab6）的直觉来源。

## 任务

### Task 1 — Trie（`primer/trie.h`）

| 组件 | 说明 |
|---|---|
| `TrieNode` | 前缀树节点：`char` → `std::map<char, std::unique_ptr<TrieNode>>` 子节点表 |
| `TrieNodeWithValue` | 带值的叶子：额外持有 `std::shared_ptr<T>` |
| `Trie::Put / Get / Remove` | 增 / 查 / 删；Put 返回**新的 Trie**（不可变数据结构） |

要点：

- `Put` 只 Clone 路径上的节点与换值的叶子，分叉出去、没碰到的子树用 `std::move` 挪进新树；
- `Remove` 递归返回时剪枝：子节点"不再是终点且没有任何孩子"就整个释放，判断信息从递归返回值带出来。

### Task 2 — TrieStore（`primer/trie_store.h`）

`Get / Put / Remove`：读写锁保护下的共享根，Get 返回快照。要点：

- 读锁内只做 `root_` 拷贝（一次原子引用计数加一），锁外慢慢查——查找可能很慢，握着读锁会拖延换根；
- `TrieStore::root_` 与 `Trie::root_` 类型不同（可变 vs 不可变），两个方向各一次标准转换：
  取快照 `std::static_pointer_cast<const TrieNode>(root_)`，
  换根 `std::const_pointer_cast<TrieNode>(new_trie.root_)`（`Trie` 已声明 `friend class TrieStore`）。

## 测试

```bash
cmake --build build --target trie_test trie_store_test --parallel $(nproc)
./build/test/trie_test && ./build/test/trie_store_test
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| trie_test 全部通过 | 60 |
| trie_store_test 全部通过 | 30 |
| 代码风格与注释（RAII 使用规范、无裸 new/delete） | 10 |

**扣分项**：快照隔离错误（写影响已读快照）-30；内存泄漏（ASAN 报告）-10。

## FAQ

**Q: Put 里到底哪些节点要 Clone，哪些可以直接 move？**
A: 只有"要改的节点"（路径上的节点 + 换值的叶子）需要 Clone；分叉出去、没碰到的子树用 `std::move` 把 unique_ptr 挪进新树。Clone 多了正确但浪费，Clone 少了会把旧树改坏（快照测试会红）。

**Q: Remove 什么时候剪枝？**
A: 递归返回时若发现子节点"不再是终点且没有任何孩子"，就把这个子节点整个释放。判断信息要从递归返回值带出来。

**Q: TrieStore 的 Get 为什么不在读锁里查找？**
A: 查找可能很慢，握着读锁会拖延换根。正确姿势：读锁内只做 `root_` 拷贝（一次原子引用计数加一），锁外慢慢查。

**Q: TrieStore 的 root_ 和 Trie 的 root_ 类型不一样，怎么换根？**
A: `TrieStore::root_` 是 `shared_ptr<TrieNode>`（可换），`Trie::root_` 是 `shared_ptr<const TrieNode>`（不可变）。见 Task 2 要点里的两次标准转换。
