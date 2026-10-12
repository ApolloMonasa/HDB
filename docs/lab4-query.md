# Lab 4：SQL 解析与查询执行

> 对标 CMU 15-445 Project 3。前面三个 Lab 造好了发动机，这个 Lab 把方向盘、变速箱装上——让用户写 SQL，让数据库跑起来。

## 概述

补全 SELECT 高级子句的解析，并实现全部 12 个火山模型执行器与 2 条优化规则。
完成后 `hsql` 交互终端可以执行完整的查询语句，端到端 `.slt` 用例全绿。

本 Lab 涉及的文件（接口见同名头文件）：

| 文件 | 说明 |
|---|---|
| `src/sql/parser.cpp` | Task 1：高级子句产生式（WHERE 表达式/ORDER BY/GROUP BY/JOIN），半给定骨架 |
| `src/execution/executors/*.cpp` | Task 2：12 个执行器 |
| `src/execution/agg_hash_table.cpp` | Task 2：`SimpleAggregationHashTable`（聚合执行器的骨架） |
| `src/optimizer/optimizer.cpp` | Task 3：两条优化规则，半给定骨架 |
| `src/include/hdb/sql/{token,ast,parser}.h` | 给定：Token / AST / 递归下降框架与基础语句 |
| `src/include/hdb/execution/{plans,expressions}/` | 给定：12 种计划节点与表达式体系 |
| `src/execution/{planner,execution_engine,executor_factory}.cpp` | 给定：计划生成与火山循环驱动 |

**注意**：`parser.cpp` / `optimizer.cpp` 内含给定骨架，改动给定部分需有充分理由并在报告中说明。

## 开始之前

- 前置：Lab2 的 TableHeap（扫描与增删改的底层）与 Lab3 的 B+ 树（IndexScan 用）；
- 通读 `execution/executors/abstract_executor.h`（Init/Next 协议）与
  `executor_common.h` 的 `ProjectTuple` 辅助；
- 跑一遍测试确认基线（`parser_test` 基础用例开局即绿）：

```bash
ctest --test-dir build -R 'parser_test|executor_test|SQLLogicTest'
```

## 背景

框架已给定的流水线：Lexer（切 Token）→ Parser 基础语句（建 AST）→
Binder（绑定 Catalog）→ Planner（生成计划树）→ Optimizer（规则改写）。
你要补全 SELECT 高级子句的解析，并实现全部执行器。

执行模型为**火山模型（Volcano / iterator model）**：每个算子实现
`Init() + Next(Tuple*)`，上层算子循环拉取下层元组。

### 教学约定（判卷以这些约定为准，实现前先读）

1. **聚合查询的 SELECT 项顺序**：先 GROUP BY 列、后聚合项
   （`SELECT dept, COUNT(id)` 合法；`SELECT COUNT(id), dept` 不在支持范围——
   计划器按"分组列 + 聚合值"的顺序拼输出行）。
2. **GROUP BY 的输出顺序**：按组键升序排列。SQL 标准不保证无 ORDER BY 时
   的输出顺序，但判卷需要确定性——你的 AggregationExecutor 应在输出前
   对分组结果按键排序。
3. **HAVING 只支持 `聚合函数(列) 比较运算符 常量` 形态**
   （如 `HAVING COUNT(id) > 2`）；对分组列的普通谓词请写进 WHERE。
4. **SELECT \* 与投影**：`*` 展开为全部列；普通列选择由最外层算子按
   OutputSchema 裁剪（参考 `executor_common.h` 的 `ProjectTuple` 辅助）。

## 任务

### Task 1 — 高级子句解析（`sql/parser.h` 中标注 TODO 的产生式）

- WHERE 里的表达式语法（比较 / 算术 / AND / OR / NOT）
- ORDER BY、GROUP BY、JOIN 子句

### Task 2 — 12 个执行器（`execution/executors/`）

| 执行器 | 说明 |
|---|---|
| `SeqScanExecutor` | 顺序扫描 + 融合谓词过滤与投影（BusTub 原版设计） |
| `InsertExecutor` | 插入并维护索引 |
| `DeleteExecutor` | 墓碑删除并维护索引 |
| `UpdateExecutor` | 主键不可变约束下的先删后插 |
| `IndexScanExecutor` | 点查 + 范围扫描 |
| `NestedLoopJoinExecutor` | inner / left join |
| `NestedIndexJoinExecutor` | 外表驱动 + 内表走索引 |
| `HashJoinExecutor` | 等值连接（Grace 落盘选做） |
| `AggregationExecutor` | GROUP BY / HAVING / DISTINCT（用 `SimpleAggregationHashTable`） |
| `SortExecutor` | 内存排序；二路外排选做 |
| `LimitExecutor` | 截断输出 |
| `TopNExecutor` | 维护大小为 N 的堆 |

### Task 3 — 优化规则（`optimizer/optimizer.cpp`）

- `SeqScan → IndexScan`（谓词命中索引时）
- `NLJ → HashJoin`（连接条件为等值时）

选做（2025 特性）：窗口函数执行器；向量化批处理改造。

## 测试

| 测试 | 覆盖点 |
|---|---|
| `test/sql/parser_test.cpp` | 高级子句解析与错误定位 |
| `test/execution/executor_test.cpp` | 各执行器语义（join 语义、聚合空组、update 约束） |
| `test/sql/*.slt` | 端到端 SQL（5 个文件 49 条记录） |

```bash
ctest --test-dir build -R 'parser_test|executor_test|SQLLogicTest'
./build/bin/hsql   # 手工交互验证
```

## 评分细则（100 分）

| 项 | 分值 |
|---|---|
| parser_test（高级子句部分） | 10 |
| 执行器：SeqScan/Insert/Delete/Update/IndexScan | 25 |
| 执行器：NLJ（含 LEFT）/HashJoin | 15 |
| 执行器：Aggregation（GROUP/HAVING/DISTINCT） | 15 |
| 执行器：Sort/Limit/TopN | 10 |
| 优化规则 R1/R2 触发验证（EXPLAIN 截图） | 10 |
| test/sql/*.slt 端到端全过 | 15 |

**扣分项**：聚合无 GROUP BY 空输入不输出 COUNT=0 行 -5；主键约束缺失 -10。

## FAQ

**Q: SeqScan 的"融合投影"具体怎么做？**
A: 拉到一行后，按本算子 OutputSchema 的各列（记录的是列下标）从原元组取值，构造新 Tuple 返回。输出元组的 schema 与表 schema 可以是"重排的子集"。

**Q: LEFT JOIN 补 NULL 行，Tuple 怎么表示 NULL？**
A: HDB 的 Value 自带 is_null_ 标志，右槽各列用 `ValueFactory::CreateNull()` 填充即可；输出 ToString 会打出 NULL。

**Q: 聚合无 GROUP BY 且输入为空，COUNT 该输出什么？**
A: 输出一行 COUNT=0（SUM/MIN/MAX 为 NULL）。这是 SQL 语义，不是实现偷懒——想想 `SELECT COUNT(*) FROM empty_table` 在 MySQL 里返回什么。

**Q: TopN 的堆方向搞反了什么现象？**
A: `ORDER BY a LIMIT 3` 想要最小 3 个 a，应维护**最大堆**（堆顶是"目前入选里最差的"），超过 N 个且比堆顶更优才换人。方向反了拿到的是最大 3 个。

**Q: UPDATE 为什么必须"先删后插"而不是原地覆盖？**
A: 变长列（VARCHAR）的新值可能比旧值长，原槽放不下。`TablePage::UpdateTupleInPlace` 只处理"新长度 <= 旧长度"的原地覆盖；放不下就走 MarkDelete + InsertTuple，主键索引同步删旧插新——注意主键列不允许出现在 SET 里。
