# Lab 5：事务与并发控制（2PL）

> 国内教材《数据库系统概论》并发控制一章主讲封锁协议——本实验把教材里的 S/X 锁、两阶段封锁、死锁逐一落到代码。

## 概述

实现 2PL 锁管理器与事务管理器，并把执行器接入事务（S2PL）。
完成后并发执行的多条 SQL 之间具备可串行化保证。

本 Lab 涉及的文件（接口见同名头文件）：

| 文件 | 说明 |
|---|---|
| `src/concurrency/lock_manager.cpp` | Task 1–2：锁请求队列 / 兼容矩阵 / FIFO / 升级 / 2PL 检查 |
| `src/concurrency/transaction_manager.cpp` | Task 3：事务生命周期与锁释放 |
| `src/include/hdb/concurrency/transaction.h` | 给定：事务对象与状态机 |
| `src/include/hdb/hdb_instance.h` | 给定：`SetTxnEnabled(true)` 开启事务模式 |

## 开始之前

- 前置：Lab4 的执行器（Task 3 要让扫描算子加 S 锁、写算子加 X 锁）；
- 通读 `lock_manager.h` 顶部的队列与升级规则说明；
- 跑一遍测试确认基线：

```bash
ctest --test-dir build -R 'lock_manager_test|transaction_test'
```

## 背景

多个事务并发读写同一数据时会互相干扰（丢失修改 / 脏读 / 不可重复读）。
2PL（两阶段封锁）是教科书给出的经典答案：
事务分为**加锁阶段**（只加不放）与**解锁阶段**（只放不加），
严格 2PL（S2PL）要求锁持有到事务结束。

## 任务

### Task 1 — 锁管理器（`concurrency/lock_manager.cpp`）

`LockShared / LockExclusive / LockUpgrade / Unlock`：表级锁请求队列。

- S 多读 X 独占（兼容矩阵）；同队列 **FIFO** 防饿死——新请求与已持锁者兼容
  也要排队，否则写者可能永远等不到；
- 升级独占通道：升级者已持 S，若和普通请求一起排队会"自己等自己"。
  队列标记 upgrading_，其他请求全部让它先走；两个事务同时升级是死锁，
  必须抛 `LockAbortException` 牺牲一个。

### Task 2 — 2PL 合法性检查

事务进入收缩期后再加锁 → 抛 `LockAbortException`（回滚重启）。
`Unlock` 在 GROWING 期第一次调用即把事务状态切到 SHRINKING（2PL 阶段分界点）。

### Task 3 — 事务管理器与执行器接入（`concurrency/transaction_manager.cpp`）

- `Begin / Commit / Abort` 事务生命周期；Commit/Abort 释放全部锁
  （漏放 = 下一个事务永久阻塞）；
- `HdbInstance::SetTxnEnabled(true)` 后，每条 DML/SELECT 包在一个事务里——
  扫描算子加 S 锁、写算子加 X 锁，Commit/Abort 时统一释放（S2PL）。

选做：等待图 + 环形检测的死锁检测（打破环时选最年轻事务牺牲）。

## 测试

| 测试 | 覆盖点 |
|---|---|
| `test/concurrency/lock_manager_test.cpp` | 兼容矩阵 / FIFO / 升级 / 2PL 违规 |
| `test/concurrency/transaction_test.cpp` | 生命周期与锁释放 |

```bash
ctest --test-dir build -R 'lock_manager_test|transaction_test'
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| lock_manager_test（兼容矩阵/FIFO/升级/2PL 违规） | 45 |
| transaction_test（生命周期与锁释放） | 25 |
| Task 3 执行器接事务（SetTxnEnabled 后并发 SQL 用例过） | 20 |
| 死锁场景分析与实验报告 | 10 |

**加分项**：等待图死锁检测（选做）+15。
**扣分项**：锁未在 Commit/Abort 全释放（下一事务永久阻塞）-20。

## FAQ

**Q: 新请求与已持锁者兼容，为什么还要排队？**
A: 防饿死。如果 S 请求总能插到等待的 X 前面，写者可能永远等不到。FIFO 下新 S 必须排在 X 之后——哪怕它与当前持锁者兼容。

**Q: 升级为什么要特殊处理？**
A: 见 Task 1 要点——"自己等自己"问题与双升级死锁。

**Q: Unlock 在 GROWING 期第一次调用要做什么？**
A: 把事务状态切到 SHRINKING（2PL 的阶段分界点），之后该事务的任何加锁请求都应抛 LockAbortException。
