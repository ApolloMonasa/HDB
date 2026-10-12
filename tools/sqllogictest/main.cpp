/**
 * tools/sqllogictest —— 简版 SQLLogicTest runner（框架给定）
 *
 * 用法：./build/bin/hdb-sqllogictest test/sql/p3.01-basic.slt [...]
 *
 * .slt 文件格式（受 SQLite sqllogictest 启发的精简子集）：
 *
 *   # 注释行
 *   statement ok
 *   CREATE TABLE t (a INT)
 *
 *   statement error
 *   SELECT no_such_col FROM t
 *
 *   query
 *   SELECT a FROM t ORDER BY a
 *   ----
 *   1
 *   2
 *
 * 退出码：全部通过 0，否则 1。
 */
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "hdb/common/exception.h"
#include "hdb/hdb_instance.h"

using namespace hdb;

namespace {

std::vector<std::string> SplitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

/** 运行一个 .slt 文件；返回失败的记录数 */
int RunFile(HdbInstance *db, const std::string &path) {
    std::ifstream f(path);
    if (!f) {
        std::printf("cannot open %s\n", path.c_str());
        return 1;
    }
    std::stringstream buf;
    buf << f.rdbuf();
    auto lines = SplitLines(buf.str());

    int failures = 0, records = 0;
    size_t i = 0;
    while (i < lines.size()) {
        std::string line = lines[i];
        // 去注释与空行
        if (line.empty() || line[0] == '#') {
            i++;
            continue;
        }
        if (line == "statement ok" || line == "statement error") {
            bool expect_ok = (line == "statement ok");
            std::string sql = lines[++i];
            records++;
            SqlResult r = db->ExecuteSql(sql);
            if (r.ok != expect_ok) {
                std::printf("FAIL [%s:%zu] statement %s: %s -> %s\n", path.c_str(), i,
                            expect_ok ? "ok" : "error", sql.c_str(),
                            r.ok ? "ok" : r.error.c_str());
                failures++;
            }
            i++;
        } else if (line == "query") {
            std::string sql = lines[++i];
            // 吃掉 ---- 分隔行与期望输出（直到空行或文件尾）
            i++;  // 此刻 i 指向 ---- 行
            if (i < lines.size() && lines[i] == "----") {
                i++;  // 真正跳过分隔行
            }
            std::vector<std::string> expect;
            while (i < lines.size() && !lines[i].empty()) {
                expect.push_back(lines[i]);
                i++;
            }
            records++;
            SqlResult r = db->ExecuteSql(sql);
            std::vector<std::string> got = r.ok ? r.RenderRows() : std::vector<std::string>{};
            if (!r.ok || got != expect) {
                std::printf("FAIL [%s] query: %s\n", path.c_str(), sql.c_str());
                if (!r.ok) {
                    std::printf("  error: %s\n", r.error.c_str());
                } else {
                    for (const auto &g : got) {
                        std::printf("  got> [%s] len=%zu\n", g.c_str(), g.size());
                    }
                    for (const auto &e : expect) {
                        std::printf("  exp> [%s] len=%zu\n", e.c_str(), e.size());
                    }
                }
                failures++;
            }
            i++;
        } else {
            std::printf("WARN unknown directive: %s\n", line.c_str());
            i++;
        }
    }
    std::printf("%s: %d records, %d failures\n", path.c_str(), records, failures);
    return failures;
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::printf("usage: %s <file.slt> [...]\n", argv[0]);
        return 1;
    }
    // 每个文件一个全新实例：文件之间状态隔离（同名建表互不冲突），
    // 与学生单文件自测的行为一致。
    int total_failures = 0;
    for (int a = 1; a < argc; a++) {
        const char *db = "/tmp/hdb_sqllogictest.db";
        std::remove(db);
        std::unique_ptr<HdbInstance> instance;
        try {
            instance = std::make_unique<HdbInstance>(db);
        } catch (const HDBException &e) {
            std::printf("HDB kernel is not fully implemented yet:\n  %s\n", e.what());
            std::remove(db);
            return 1;
        }
        total_failures += RunFile(instance.get(), argv[a]);
        std::remove(db);
    }
    return total_failures == 0 ? 0 : 1;
}
