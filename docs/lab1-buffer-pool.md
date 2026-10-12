# Lab 1：磁盘与缓冲池

> 对标 CMU 15-445 Project 1。这是你第一次接触数据库内核最底层的两个组件：把"页"从磁盘搬进内存（谁搬、何时搬、满了淘汰谁）。

## 概述

实现磁盘调度与缓冲池的四个组件。完成本 Lab 后，上层组件可以通过
`BufferPoolManager` 透明地读写"内存中的页"，完全不必关心缺页与淘汰。

本 Lab 涉及的文件（均为实现位，接口见同名头文件）：

| 文件 | 说明 |
|---|---|
| `src/buffer/lru_k_replacer.cpp` | Task 1：LRU-K 替换器 |
| `src/storage/disk/disk_scheduler.cpp` | Task 2：磁盘调度器（后台线程 + 请求队列） |
| `src/buffer/buffer_pool_manager.cpp` | Task 3：缓冲池管理器 |
| `src/storage/page/page_guard.cpp` | Task 4：Read/WritePageGuard（RAII 页守卫） |
| `src/storage/disk/disk_manager.cpp` | 给定：文件级页读写，`disk_manager_test` 开局即绿 |
| `src/include/hdb/common/channel.h` | 给定：线程安全队列（DiskScheduler 的请求通道） |

## 开始之前

- 前置：Lab0 的移动语义与读写锁会直接用到；
- 通读 `common/config.h`（页大小 4KB、ID 类型）与 `storage/page/page.h`（Page 的内存布局）；
- 跑一遍测试确认基线（`disk_manager_test` 应通过，其余失败是预期）：

```bash
ctest --test-dir build -R 'disk_manager_test|lru_k_replacer_test|disk_scheduler_test|buffer_pool_manager_test|page_guard_test'
```

## 背景

数据库文件远大于内存。所有上层组件（表、索引）都只面对**内存中的页**，
缺页时由缓冲池负责从磁盘调入；内存页满时按替换策略淘汰。
**HDB 中一切页面访问必须经过 BufferPoolManager**——这是贯穿后续所有 Lab 的铁律。

页大小为 4KB（`hdb::PAGE_SIZE`，见 `common/config.h`），与教材中"磁盘块"对齐。

## 任务

按依赖顺序实现四个组件（对应 BusTub P1 的四个 Task）：

### Task 1 — LRU-K 替换器（`buffer/lru_k_replacer.h`）

- 记录每个 frame 最近 K 次访问时间戳；淘汰时优先驱逐 **backward k-distance 最远** 的帧；
- 没有满 K 次访问的帧按普通 LRU（比最早时间戳）参与淘汰；
- `SetEvictable` 可动态改变帧的可驱逐状态，`Evict()` 只在可驱逐帧里选；
- 线程安全（一把互斥锁即可，不要求优化）。

### Task 2 — 磁盘调度器（`storage/disk/disk_scheduler.h`）

- `Schedule(DiskRequest)` 把读/写请求投递到队列，由**后台线程**异步执行；
- 请求完成/失败后通过 `std::promise` 通知调用方（`callback.set_value()`）；
- 队列使用框架给定的 `common/channel.h`；
- 析构时优雅退出后台线程。

### Task 3 — 缓冲池管理器（`buffer/buffer_pool_manager.h`）

- `NewPage` / `FetchPage` / `UnpinPage` / `FlushPage` / `DeletePage`；
- 页表：`page_id → frame`；空闲帧耗尽时先问 Replacer 要一个可驱逐帧；
- 被驱逐页若 dirty 必须先写回磁盘；pin 计数为 0 的页才可被驱逐。

### Task 4 — 页守卫（`storage/page/page_guard.h`）

- `ReadPageGuard` / `WritePageGuard`：RAII 封装"持有页 + 正确的锁 + 归还时 Unpin"；
- 实现移动构造/移动赋值（守卫不可拷贝），析构自动 Drop；
- `WritePageGuard::Drop()` 负责置脏；`Flush()` 立即落盘。

## 测试

| 测试 | 覆盖点 |
|---|---|
| `test/buffer/lru_k_replacer_test.cpp` | k 距离语义、evictable 标志、时间戳 |
| `test/storage/disk_scheduler_test.cpp` | 异步读写回调、并发投递、优雅退出 |
| `test/buffer/buffer_pool_manager_test.cpp` | 页表/驱逐/脏页写回/pin 计数守恒 |
| `test/storage/page_guard_test.cpp` | RAII 自动归还、移动语义无双重释放 |
| `test/storage/disk_manager_test.cpp` | 给定组件自测（开局即绿） |

```bash
ctest --test-dir build -R 'lru_k_replacer_test|disk_scheduler_test|buffer_pool_manager_test|page_guard_test'
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| lru_k_replacer_test | 20 |
| disk_scheduler_test | 15 |
| buffer_pool_manager_test | 25 |
| page_guard_test | 15 |
| 驱逐路径正确性代码走查（脏页写回顺序） | 15 |
| 移动语义无双重释放（ASAN 干净） | 10 |

**扣分项**：全局锁包裹整个缓冲池（并发测试超时）-15；后台线程未优雅退出（析构挂起）-10。

## FAQ

**Q: 驱逐时到底先做什么？**
A: 顺序是——脏页先写回磁盘（经 DiskScheduler）→ 从 page_table_ 摘除旧 page_id → 换上新 page_id → 清零页数据 → 置 is_dirty_ = false。顺序错了会出现"读旧页号拿到新页数据"的诡异 bug。

**Q: promise 忘了 set_value 会怎样？**
A: 调用方 `future.get()` 永久挂起，测试超时。DiskRequest 无论成败都要 set——失败 set(false) 也是"回电"。

**Q: 移动后的 PageGuard 析构为什么不能 Drop？**
A: 移动构造把 bpm_/page_ 置空后，析构看到空守卫直接返回即可。忘了置空 = 同一页被 unpin 两次，pin 计数变负，后续驱逐逻辑全乱。

**Q: UnpinPage 的 is_dirty 和页上已有的脏标记是什么关系？**
A: 取"或"。页可能先被读（false）后又写了（true），任何一次说脏它就是脏的。

**Q: FetchPage 缺页路径里 ResetMemory 和读盘的顺序有讲究吗？**
A: 有：**先 `ResetMemory()` 再发起磁盘读**。读进来的数据落在帧的数据区里，顺序颠倒会把刚读到的数据清零——症状是"驱逐后再 Fetch 拿到空页"，`page_guard_test` 的 WriteGuardAutoDrop 会抓到。

**Q: 驱逐的时候要考虑页上的 latch 吗？**
A: HDB 的骨架实现不需要：可驱逐页 pin 计数为 0，没有守卫持有它，也就没人持有它的页锁。真实系统里驱逐与页锁的交互是著名的难题（buffer pool 与 latch 的耦合问题，可作拓展阅读）。
