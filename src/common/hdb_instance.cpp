#include "hdb/hdb_instance.h"

#include "hdb/execution/explain.h"
#include "hdb/optimizer/optimizer.h"
#include "hdb/sql/lexer.h"
#include "hdb/sql/parser.h"

namespace hdb {

std::vector<std::string> SqlResult::RenderRows() const {
    std::vector<std::string> lines;
    for (const auto &t : rows) {
        std::string line;
        for (uint32_t c = 0; c < row_schema.GetColumnCount(); c++) {
            if (c > 0) {
                line += "|";
            }
            line += t.GetValue(row_schema, c).ToString();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

HdbInstance::HdbInstance(const std::string &db_file)
    : dm_(std::make_unique<DiskManager>(db_file)),
      bpm_(std::make_unique<BufferPoolManager>(64, dm_.get())),
      catalog_(std::make_unique<Catalog>(bpm_.get())),
      planner_(catalog_.get()) {}

HdbInstance::~HdbInstance() = default;

SqlResult HdbInstance::ExecuteSql(const std::string &sql) {
    SqlResult result;
    try {
        sql::Lexer lexer(sql);
        sql::Parser parser(lexer.Tokenize());
        auto stmt = parser.ParseStatement();

        switch (stmt->type) {
            case sql::Statement::Type::CREATE:
                return HandleCreate(stmt->create.get());
            case sql::Statement::Type::DROP:
                return HandleDrop(stmt->drop.get());
            default:
                break;
        }

        auto plan = planner_.Plan(stmt.get());
        plan = Optimizer::Optimize(std::move(plan));

        if (stmt->is_explain) {
            result.ok = true;
            result.explain_plan = PlanToString(plan.get());
            return result;
        }

        Transaction *txn = nullptr;
        if (txn_enabled_) {
            txn = txn_manager_.Begin();  // Lab5 Task3：语句级事务（S2PL）
        }
        try {
            ExecutorContext ctx(catalog_.get(), txn,
                               txn != nullptr ? &txn_manager_.GetLockManager() : nullptr);
            int affected = engine_.Execute(plan.get(), &ctx, &result.rows);
            if (txn != nullptr) {
                txn_manager_.Commit(txn);  // 终点统一放锁
            }
            result.ok = true;
            result.affected = affected;
            result.has_rows = !result.rows.empty();
            result.row_schema = plan->OutputSchema();
        } catch (const HDBException &) {
            if (txn != nullptr) {
                txn_manager_.Abort(txn);  // 异常路径同样放锁
            }
            throw;
        }
        return result;
    } catch (const HDBException &e) {
        result.ok = false;
        result.error = e.what();
        return result;
    }
}

SqlResult HdbInstance::HandleCreate(const sql::CreateStmt *stmt) {
    SqlResult result;
    try {
        std::vector<Column> columns;
        std::string pk;
        for (const auto &def : stmt->columns) {
            Column c(def.name, def.type, def.varchar_len);
            columns.push_back(std::move(c));
            if (def.primary_key) {
                if (!pk.empty()) {
                    throw ExecutionException("only one primary key column is supported");
                }
                pk = def.name;
            }
        }
        catalog_->CreateTable(stmt->table, columns, pk);
        result.ok = true;
    } catch (const HDBException &e) {
        result.ok = false;
        result.error = e.what();
    }
    return result;
}

SqlResult HdbInstance::HandleDrop(const sql::DropStmt *stmt) {
    SqlResult result;
    try {
        catalog_->DropTable(stmt->table);
        result.ok = true;
    } catch (const HDBException &e) {
        result.ok = false;
        result.error = e.what();
    }
    return result;
}

}  // namespace hdb
