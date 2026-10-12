# Lab 2：记录存储与表管理

> BusTub 中这一层是框架给定的；HDB 把它开成实验——因为"一行记录如何在 4KB 页面里排布"正是教材"记录组织与存储"一章的核心。

## 概述

有了缓冲池，接下来回答：**关系表的一行（tuple）如何变成页里的字节？**
本 Lab 实现元组序列化、槽位页布局与表堆遍历；完成后即可通过
`TableHeap` 插入并扫描一张完整的表。

本 Lab 涉及的文件（接口见同名头文件）：

| 文件 | 说明 |
|---|---|
| `src/storage/table/tuple.cpp` | Task 1：元组序列化/反序列化 |
| `src/storage/page/table_page.cpp` | Task 2：槽位页布局与增删改查 |
| `src/storage/table/table_heap.cpp` | Task 3：页面链表组织的表 |
| `src/storage/table/table_iterator.cpp` | Task 3：跨页遍历迭代器 |
| `src/include/hdb/storage/table/{schema,column}.h` | 给定：列偏移/NULL 位图协议（Tuple 的"合同"） |
| `src/include/hdb/type/value.h` | 给定：Value 序列化（`Value::SerializeTo`） |

## 开始之前

- 前置：Lab1 的 `WritePageGuard`（TablePage 的所有修改都在守卫下进行）；
- 通读 `schema.h` 顶部关于 NULL 位图与列偏移的协议说明；
- 跑一遍测试确认基线：

```bash
ctest --test-dir build -R 'tuple_test|table_page_test|table_heap_test'
```

## 背景

教材称这种布局为 slotted page（槽位页）/ 变长记录存储：**槽位目录从前往后长，
数据区从后往前长**，两个区域相向生长，中间的空隙就是剩余空间。
变长记录不知道自己多长，从前往后插会不断搬移；从后往前长只需挪 data_begin 一个游标。

### 元组字节布局（先读懂，再动手 Tuple）

HDB 的元组采用 **NULL 位图头 + 定长槽位** 布局（与 `Schema` 的给定实现配套）：

```
[ NULL 位图 ceil(列数/8) 字节 ][ 列0 槽 ][ 列1 槽 ][ ... ]
                                 │          │
                        按列类型定宽          VARCHAR(n) 槽 = [4B 长度][内容][补零到 4+n]
```

- 每列的槽偏移由 `Schema` 构造时统一计算（`Column::GetOffset()` 已含位图头的偏移，
  槽宽见 `Column::GetFixedSerializedSize()`）——**你的 Tuple 不需要自己算偏移**；
- 第 i 列为 NULL 时：置位图第 i 位（`Schema::SetColumnNull`），槽内留零，**不要调用
  `Value::SerializeTo`**（NULL 值的 Value 类型标签是 INVALID，序列化会抛异常）；
- `GetValue` 读值前先查 `Schema::ColumnIsNull`，命中则直接返回 NULL 值。

这套协议是 Tuple/Schema/TablePage 三方共同遵守的"合同"——照着实现，
`tuple_test` 的序列化往返与 NULL 用例即可通过。

## 任务

### Task 1 — Tuple 序列化（`storage/table/tuple.cpp`）

`SerializeTo / DeserializeFrom`：按 Schema 的列偏移把元组序列化成字节流。
VARCHAR 实际占用随内容变化，元组长度要自己记录。

### Task 2 — TablePage（`storage/page/table_page.cpp`）

页头 + 槽位目录（从前往后长）+ 数据区（从后往前长）：
`InsertTuple / MarkDelete / GetTuple / UpdateTuple`。
页满时插入返回失败；删除采用**墓碑标记**（空间留给后续实验回收）。

### Task 3 — TableHeap 与 TableIterator（`storage/table/`）

表 = 页面链表；维护首个/最后一个页面，提供迭代器。
迭代器遍历整张表（跨页移动，跳过已删除记录）。

## 测试

| 测试 | 覆盖点 |
|---|---|
| `test/table/tuple_test.cpp` | 序列化往返、NULL 位图、变长 VARCHAR |
| `test/storage/table_page_test.cpp` | 槽位布局、页满拒绝、墓碑语义、原地更新 |
| `test/table/table_heap_test.cpp` | 跨页插入、迭代器遍历、删除不可见 |

```bash
ctest --test-dir build -R 'tuple_test|table_page_test|table_heap_test'
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| tuple_test | 15 |
| table_page_test | 30 |
| table_heap_test（含跨页与迭代） | 35 |
| 布局图与设计说明（实验报告） | 10 |
| 墓碑语义正确（删除即不可见） | 10 |

**加分项**：实现页内空间整理（compaction）+10。

## FAQ

**Q: 为什么数据区从页尾往前长，槽位目录从前往后长？**
A: 变长记录不知道自己多长，从前往后插会不断搬移；从后往前长只需挪 data_begin 一个游标。两个区域相向生长，中间的空隙就是剩余空间——教材"变长记录的页组织"的标准答案。

**Q: 墓碑删除后空间不回收，页不是越来越虚？**
A: 是。真实系统会做页内 compaction / 空间整理，HDB 把它留作加分项。

**Q: Tuple 的长度为什么要自己记？**
A: VARCHAR 实际占用随内容变化，Schema 只能给上限。不知道实际长度就没法把字节从页里完整拷出来。
