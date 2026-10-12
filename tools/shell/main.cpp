/**
 * hsql —— HDB 交互式终端（框架给定；英文界面，规避终端编码问题）
 *
 * 用法：./build/bin/hsql [db_file]
 *   hdb> CREATE TABLE t (a INT PRIMARY KEY, b VARCHAR(8));
 *   ok
 *   hdb> INSERT INTO t VALUES (1, 'x');
 *   ok (1 row)
 *   hdb> SELECT * FROM t;
 *   a | b
 *   1 | x
 */
#include <cstdio>
#include <iostream>
#include <string>

#include <memory>

#include "hdb/common/config.h"
#include "hdb/common/exception.h"
#include "hdb/hdb_instance.h"

using namespace hdb;

int main(int argc, char **argv) {
    std::string db_file = argc > 1 ? argv[1] : "/tmp/hdb_default.db";
    // 与 BusTub shell 一致的提示符风格
    std::printf("Welcome to HDB v%s (HDU educational database kernel).\n", HDB_VERSION);
    std::printf("Type SQL statements; quit with 'quit;'\n");
    std::printf("Attached database file: %s\n\n", db_file.c_str());

    std::unique_ptr<HdbInstance> db;
    try {
        db = std::make_unique<HdbInstance>(db_file);
    } catch (const HDBException &e) {
        std::printf("HDB kernel is not fully implemented yet:\n  %s\n", e.what());
        std::printf("Finish the labs above to bring the shell to life.\n");
        return 1;
    }
    std::string line;
    std::printf("hsql> ");
    while (std::getline(std::cin, line)) {
        if (line == "quit;" || line == "quit" || line == "exit") {
            break;
        }
        if (line.empty()) {
            std::printf("hsql> ");
            continue;
        }
        SqlResult r = db->ExecuteSql(line);
        if (!r.ok) {
            std::printf("ERROR: %s\n", r.error.c_str());
        } else if (!r.explain_plan.empty()) {
            std::printf("%s", r.explain_plan.c_str());
        } else if (r.has_rows) {
            // 表头
            const auto &cols = r.row_schema.GetColumns();
            for (size_t c = 0; c < cols.size(); c++) {
                if (c > 0) {
                    std::printf(" | ");
                }
                std::printf("%s", cols[c].GetName().c_str());
            }
            std::printf("\n");
            for (const auto &row : r.RenderRows()) {
                std::printf("%s\n", row.c_str());
            }
        } else {
            std::printf("ok (%d rows affected)\n", r.affected);
        }
        std::printf("\nhdb> ");
    }
    std::printf("bye\n");
    return 0;
}
