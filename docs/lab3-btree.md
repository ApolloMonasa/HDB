# Lab 3：B+ 树索引

> 对标 CMU 15-445 Project 2（Fall 2024 形态）。B+ 树是数据库索引的标准答案：所有数据在叶子、内部节点只做导航、叶子串成链表。

## 概述

实现页面级的 B+ 树：增删查、范围扫描迭代器、**并发蟹行加锁（必做）**。
完成后 `IndexScan` 可以走树导航，而不是全表扫描。

本 Lab 涉及的文件（接口见同名头文件）：

| 文件 | 说明 |
|---|---|
| `src/storage/page/b_plus_tree_leaf_page.cpp` | Task 1：叶子页（key, RID） |
| `src/storage/page/b_plus_tree_internal_page.cpp` | Task 1：内部页（key, page_id）导航 |
| `src/storage/index/b_plus_tree.cpp` | Task 2：树的增删查与分裂/合并/重分配 |
| `src/storage/index/index_iterator.cpp` | Task 3：沿叶子链表的范围扫描迭代器 |
| `src/include/hdb/storage/page/b_plus_tree_page.h` | 给定：页面基类（页头字段与通用断言） |
| `src/include/hdb/storage/index/generic_key.h` | 给定：定长索引键的序列化与比较 |
| `tools/b_plus_tree_printer/` | 调试利器：`./build/bin/hdb-bpt-printer` 把树导出成 DOT/文本画像 |

## 开始之前

- 前置：Lab1 的 `ReadPageGuard` / `WritePageGuard`（蟹行加锁的基本工具）；
- 通读 `b_plus_tree.h` 顶部的并发规则说明与 `b_plus_tree_page.h` 的页头布局；
- 跑一遍测试确认基线：

```bash
ctest --test-dir build -R b_plus_tree
```

## 背景

和表一样，**B+ 树本身也是一页一页存放在缓冲池里的**：
根 ID 放在 Header 页，内部页存 (key, page_id) 导航项，叶子页存 (key, RID)。
你在 Lab1 实现的 PageGuard 在这里大量使用——向下走锁子页、向上放父页（蟹行加锁，crabing）：
持有父页锁的同时只锁一个"可能被修改"的子页，判断子页安全后立即放父页，
锁的移动像螃蟹走路。读路径用 ReadPageGuard 蟹行（放得更早），
写路径用 WritePageGuard 蟹行。

## 任务

### Task 1 — 叶子页与内部页（`storage/page/`）

- 叶子页 `BPlusTreeLeafPage`：二分查找 key、插入（满了分裂给兄弟）、与前后叶子互链；
- 内部页 `BPlusTreeInternalPage`：key 数 = 子页数 - 1；查找、分裂时把中间 key 上推；
- 分裂时中间键的去向：叶子分裂**复制**上推（左叶子保留一份，因为叶子存数据），
  内部页分裂**移动**上推（它只负责导航，留在原页会重复）。

### Task 2 — 树操作与并发（`storage/index/b_plus_tree.cpp`）

- `Insert / GetValue / Remove`：自顶向下；删除时先试重分配（从兄弟借），
  兄弟也给不动才合并（重分配不改变树高，合并会向上传播——能不传播就不传播）；
- 并发（必做）：乐观蟹行加锁——插入时子页未满（size < max_size - 1）、
  删除时子页够满（size > min_size）即"安全"，安全的子页不会再分裂/合并，父页即可释放。

### Task 3 — IndexIterator（`storage/index/index_iterator.cpp`）

沿叶子链表做范围扫描（C++ 迭代器接口，`begin()/end()` 语义）。

选做（2025 特性）：叶子页 tombstone 删除缓冲。

## 测试

| 测试 | 覆盖点 |
|---|---|
| `test/storage/b_plus_tree_insert_test.cpp` | 顺序/乱序插入、分裂正确性 |
| `test/storage/b_plus_tree_delete_test.cpp` | 合并/重分配、删到空树 |
| `test/storage/b_plus_tree_concurrent_test.cpp` | 多线程无死锁、无丢失更新 |
| `test/storage/b_plus_tree_sequential_scale_test.cpp` | 大规模与范围扫描 |

```bash
ctest --test-dir build -R b_plus_tree
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| b_plus_tree_insert_test（含乱序大规模） | 25 |
| b_plus_tree_delete_test（含删到空树） | 20 |
| b_plus_tree_concurrent_test（蟹行必做） | 20 |
| b_plus_tree_sequential_scale_test（范围扫描） | 10 |
| 树画像/printer 调试截图与分裂分析（报告） | 15 |
| 二分查找（禁止线性扫描） | 10 |

**扣分项**：全树持根锁（并发形同串行）-15；死锁 -20。
**加分项**：tombstone 删除缓冲 +10；TSAN 干净 +5。

## FAQ

**Q: 蟹行加锁什么时候才能放父页？**
A: 判断子页"安全"——见 Task 2 的判据。安全的子页不会再分裂/合并，父页自然不用改。

**Q: 删除时先重分配还是先合并？**
A: 先试重分配（从兄弟借一个），兄弟也给不动才合并。

**Q: 并发测试偶现失败但单线程全绿？**
A: 大概率是"在持有父页写锁时又去拿祖父页锁"（锁序不是单向的），或者读路径用了 WritePageGuard。上 TSAN 排查：

```bash
cmake -B build -DHDB_SANITIZER=thread
cmake --build build --target b_plus_tree_concurrent_test --parallel $(nproc)
./build/test/b_plus_tree_concurrent_test
```

**Q: 怎么肉眼看自己的树对不对？**
A: 用 `./build/bin/hdb-bpt-printer` 导出 DOT 图（Graphviz 渲染），检查分裂后键的分布与叶子链表完整性。
