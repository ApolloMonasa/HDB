# HDB —— 数据库原理实验内核框架

HDB（**H**DU **D**ata**B**ase）是一个小型关系数据库内核骨架，教学思路参考 CMU 15-445 的 [BusTub](https://github.com/cmu-db/bustub)：**头文件接口给定、学生填实现、单元测试验证**。你将从磁盘与缓冲池开始，逐层实现一个能够解析并执行 SQL 的数据库内核。

全部待实现函数只抛出 `NotImplementedException`，函数上方带中文 TODO 注释块——骨架开箱即可编译；未实现的实验对应测试失败是预期行为，完成实现后测试转绿。

## 实验一览

| Lab | 主题 | 指导书 | 单元测试 |
|---|---|---|---|
| 0 | C++ 热身：并发前缀树 | [docs/lab0-trie.md](docs/lab0-trie.md) | `test/primer/` |
| 1 | 磁盘与缓冲池 | [docs/lab1-buffer-pool.md](docs/lab1-buffer-pool.md) | `test/buffer/` `test/storage/` |
| 2 | 记录存储与表管理 | [docs/lab2-record.md](docs/lab2-record.md) | `test/table/` `test/storage/` |
| 3 | B+ 树索引（含并发蟹行） | [docs/lab3-btree.md](docs/lab3-btree.md) | `test/storage/` |
| 4 | SQL 解析与查询执行 | [docs/lab4-query.md](docs/lab4-query.md) | `test/sql/` `test/execution/` |
| 5 | 事务与并发控制（2PL） | [docs/lab5-txn.md](docs/lab5-txn.md) | `test/concurrency/` |
| 6 | MVCC（选做） | [docs/lab6-mvcc.md](docs/lab6-mvcc.md) | `test/concurrency/` |

整体分层与一次 SELECT 的完整旅程见 [docs/architecture.md](docs/architecture.md)。

## 构建

开发环境基线：Linux（Fedora / Ubuntu 均可）、CMake 3.16+、C++17；编译器 GCC 11+ 或 Clang（CI 在 ubuntu-22.04 / 24.04 + clang 矩阵上构建验证）。

先安装依赖（自动识别发行版，需 root；macOS 可直接运行）：

```bash
sudo build_support/packages.sh
```

然后配置并构建（默认 Debug + AddressSanitizer，便于排查内存错误）：

```bash
cmake -B build
cmake --build build --parallel $(nproc)
```

换用 ThreadSanitizer 排查数据竞争（Lab3 蟹行加锁 / Lab5 锁管理器推荐）：

```bash
cmake -B build -DHDB_SANITIZER=thread
cmake --build build --parallel $(nproc)
```

注意：`address` 与 `thread` 互斥，切换时建议先删除构建目录。

常用 CMake 选项（`cmake -B build -D<选项>=<值>`）：

| 选项 | 默认 | 说明 |
|---|---|---|
| `HDB_BUILD_TESTS` | ON | 构建单元测试（需要系统 GTest） |
| `HDB_BUILD_TOOLS` | ON | 构建 hsql 等工具 |
| `HDB_WERROR` | OFF | 警告视为错误 |
| `HDB_SANITIZER` | address | `address` / `thread` / `none` |

## 测试

全部测试（19 个 GTest 二进制 + 5 个端到端 SQL 用例）：

```bash
ctest --test-dir build --output-on-failure
```

只构建并运行单个测试（调试时最常用）：

```bash
cmake --build build --target lru_k_replacer_test --parallel $(nproc)
./build/test/lru_k_replacer_test
```

在单个测试里只跑部分用例（`--gtest_filter` 支持通配）：

```bash
./build/test/lru_k_replacer_test --gtest_filter='LRUKReplacerTest.KDistance*'
```

端到端 SQL 用例（`.slt` 文件，由 `hdb-sqllogictest` 执行）：

```bash
cmake --build build --target p3.01-basic_test
./build/bin/hdb-sqllogictest test/sql/p3.01-basic.slt
```

按 Lab 汇总用例通过率，一眼看到"离完成还有多远"：

```bash
./tools/progress.sh build
```

统一代码风格（需要 clang-format）：

```bash
cmake --build build --target format
```

## 使用

完成 Lab1 之后即可启动交互式 SQL 终端：

```bash
./build/bin/hsql
```

```text
hdb> CREATE TABLE student (id INT, name VARCHAR(32), PRIMARY KEY(id));
hdb> INSERT INTO student VALUES (1, 'alice');
hdb> SELECT name FROM student WHERE id = 1;
```

## 目录结构

```
HDB/
├── CMakeLists.txt              顶层构建：C++17 / 选项开关 / 子目录挂载
├── .clang-format               代码风格（format 目标自动套用）
├── .vscode/                    VSCode 配置（推荐扩展、构建任务）
│
├── build_support/
│   └── packages.sh             依赖一键安装（dnf/apt/zypper/pacman 自动识别）
│
├── docs/
│   ├── architecture.md         架构分层图 + 一次 SELECT 的时序图
│   ├── lab0-trie.md ~ lab6-mvcc.md   各 Lab 指导书
│   └── template/lab-report.md  实验报告模板
│
├── src/
│   ├── CMakeLists.txt          hdb_module() 汇总各模块为单一 libhdb.a
│   ├── include/hdb/            全部头文件——接口即"题目"，先读懂再动手
│   │   ├── common/             全局配置(页大小/ID 类型)、RID、异常、读写锁、Channel
│   │   ├── primer/             Lab0：Trie / TrieStore
│   │   ├── type/               Value 类型系统（序列化/比较/算术，给定）
│   │   ├── buffer/             Replacer 抽象 / LRUKReplacer / BufferPoolManager（Lab1）
│   │   ├── storage/
│   │   │   ├── disk/           DiskManager（给定）/ DiskScheduler（Lab1）
│   │   │   ├── page/           Page / PageGuard（Lab1）/ TablePage（Lab2）/
│   │   │   │                   B+ 树三个页面类（Lab3）
│   │   │   ├── table/          Tuple / Schema / Column / TableHeap / 迭代器（Lab2）
│   │   │   └── index/          索引抽象 / B+ 树 / GenericKey / IndexIterator（Lab3）
│   │   ├── catalog/            系统目录：表登记、主键索引联动（给定）
│   │   ├── sql/                Token / Lexer（给定）/ AST / Parser（半给定）
│   │   ├── execution/
│   │   │   ├── plans/          12 种计划节点（给定）
│   │   │   ├── expressions/    执行期表达式体系（给定）
│   │   │   └── executors/      12 个火山模型执行器 + 聚合哈希表（Lab4）
│   │   ├── optimizer/          两条优化规则 R1/R2（Lab4）
│   │   ├── concurrency/        Transaction（给定）/ LockManager / TxnManager（Lab5）
│   │   │                       / MvccManager（Lab6 选做）
│   │   └── hdb_instance.h      串起全流水线的实例（给定）
│   └── **/（与 include 镜像）   实现文件 + 每模块一份 CMakeLists.txt
│
├── test/
│   ├── primer/ buffer/ storage/ table/    按模块组织的 GTest 单元测试，
│   ├── sql/ execution/ concurrency/      每个测试文件一个独立可执行程序
│   └── sql/*.slt              端到端 SQL 用例
│
└── tools/
    ├── shell/                  hsql——交互式 SQL 终端
    ├── b_plus_tree_printer/    hdb-bpt-printer——B+ 树文本画像/DOT 导出（Lab3 调试）
    ├── sqllogictest/           hdb-sqllogictest——.slt 端到端测试 runner
    ├── benchmark/              hdb-bench——吞吐基准（缓冲池/B+树/SQL 插入）
    └── progress.sh             实验进度自查（按 Lab 汇总测试通过率）
```

构建产物统一在 `build/` 下：`bin/`（工具可执行文件）、`test/`（测试可执行文件）、`lib/`（libhdb.a）。

**调用链**：SQL 文本 → Lexer → Parser(AST) → Planner → Optimizer → Executor（火山模型）→ TableHeap → BufferPoolManager → DiskScheduler → DiskManager。

## License

[MIT](LICENSE)。教学设计参考 [cmu-db/bustub](https://github.com/cmu-db/bustub)。
