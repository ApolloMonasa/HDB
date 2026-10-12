#include <algorithm>
#include "hdb/execution/planner.h"

#include "hdb/common/exception.h"

namespace hdb {

using sql::Expr;
using sql::SelectItem;

std::unique_ptr<AbstractPlanNode> Planner::Plan(const sql::Statement *stmt) {
    switch (stmt->type) {
        case sql::Statement::Type::SELECT:
            return PlanSelect(stmt->select.get());
        case sql::Statement::Type::INSERT:
            return PlanInsert(stmt->insert.get());
        case sql::Statement::Type::DELETE_:
            return PlanDelete(stmt->del.get());
        case sql::Statement::Type::UPDATE:
            return PlanUpdate(stmt->update.get());
        default:
            throw ExecutionException("planner: statement type has no plan (DDL handled by shell)");
    }
}

// ---------------------------------------------------------------------------
// 表达式绑定（AST → 可执行表达式；名字解析在这里完成）
// ---------------------------------------------------------------------------
AbstractExpressionRef Planner::BindExpr(const Expr *expr,
                                        const std::vector<std::pair<std::string, Schema>> &scopes,
                                        int default_idx, const Schema *single_stream) {
    switch (expr->kind) {
        case Expr::Kind::LITERAL:
            return std::make_shared<ConstantValueExpression>(expr->literal);

        case Expr::Kind::COLUMN_REF: {
            int scope_idx = -1;
            if (!expr->table.empty()) {
                for (size_t i = 0; i < scopes.size(); i++) {
                    if (scopes[i].first == expr->table) {
                        scope_idx = static_cast<int>(i);
                        break;
                    }
                }
                if (scope_idx == -1) {
                    throw ExecutionException("unknown table alias: " + expr->table);
                }
            } else {
                for (size_t i = 0; i < scopes.size(); i++) {
                    if (scopes[i].second.HasColumn(expr->column)) {
                        if (scope_idx != -1) {
                            throw ExecutionException("ambiguous column: " + expr->column);
                        }
                        scope_idx = static_cast<int>(i);
                    }
                }
                if (scope_idx == -1) {
                    throw ExecutionException("unknown column: " + expr->column);
                }
                (void)default_idx;
            }
            if (single_stream != nullptr) {
                return std::make_shared<ColumnValueExpression>(
                    0, single_stream->GetColumn(expr->column).GetColumnId(), *single_stream);
            }
            const Schema &schema = scopes[scope_idx].second;
            return std::make_shared<ColumnValueExpression>(
                scope_idx, schema.GetColumn(expr->column).GetColumnId(), schema);
        }

        case Expr::Kind::BINARY_OP: {
            auto l = BindExpr(expr->left.get(), scopes, default_idx, single_stream);
            auto r = BindExpr(expr->right.get(), scopes, default_idx, single_stream);
            if (expr->op == "=" || expr->op == "<>" || expr->op == "<" || expr->op == "<=" ||
                expr->op == ">" || expr->op == ">=") {
                return std::make_shared<ComparisonExpression>(expr->op, std::move(l), std::move(r));
            }
            return std::make_shared<ArithmeticExpression>(expr->op, std::move(l), std::move(r));
        }

        case Expr::Kind::LOGIC_AND:
            return LogicExpression::And(BindExpr(expr->left.get(), scopes, default_idx, single_stream),
                                        BindExpr(expr->right.get(), scopes, default_idx, single_stream));
        case Expr::Kind::LOGIC_OR:
            return LogicExpression::Or(BindExpr(expr->left.get(), scopes, default_idx, single_stream),
                                       BindExpr(expr->right.get(), scopes, default_idx, single_stream));
        case Expr::Kind::LOGIC_NOT:
            return LogicExpression::Not(BindExpr(expr->left.get(), scopes, default_idx, single_stream));

        case Expr::Kind::AGGREGATE: {
            // COUNT(*) 绑定为 COUNT(1)（'*' 无列可查）
            if (expr->left != nullptr && expr->left->kind == Expr::Kind::STAR) {
                return std::make_shared<AggregateExpression>(AggType::COUNT,
                                                             std::make_shared<ConstantValueExpression>(
                                                                 ValueFactory::CreateInteger(1)));
            }
            AggType func;
            if (expr->op == "COUNT") {
                func = AggType::COUNT;
            } else if (expr->op == "SUM") {
                func = AggType::SUM;
            } else if (expr->op == "MIN") {
                func = AggType::MIN;
            } else if (expr->op == "MAX") {
                func = AggType::MAX;
            } else {
                func = AggType::AVG;
            }
            return std::make_shared<AggregateExpression>(
                func, BindExpr(expr->left.get(), scopes, default_idx, single_stream));
        }

        case Expr::Kind::STAR:
            throw ExecutionException("'*' is only allowed in SELECT list");
    }
    throw ExecutionException("planner: unreachable expression kind");
}

namespace {
/** 把 SELECT 项里的列引用定位到合并 schema 的 Column（给定辅助） */
const Column &FindOutColumn(const std::vector<Column> &merged, const Expr *expr) {
    if (expr->kind != Expr::Kind::COLUMN_REF) {
        throw ExecutionException("planner: non-column SELECT item without alias is unsupported");
    }
    for (const auto &c : merged) {
        if (c.GetName() == expr->column) {
            return c;
        }
    }
    throw ExecutionException("unknown column in SELECT list: " + expr->column);
}
}  // namespace

// ---------------------------------------------------------------------------
// SELECT：FROM/JOIN → WHERE 融合 → 聚合（若有）→ ORDER BY → LIMIT
// ---------------------------------------------------------------------------
std::unique_ptr<AbstractPlanNode> Planner::PlanSelect(const sql::SelectStmt *stmt) {
    TableInfo *table = catalog_->GetTable(stmt->table);
    std::string main_name = stmt->alias.empty() ? stmt->table : stmt->alias;

    std::vector<std::pair<std::string, Schema>> scopes{{main_name, table->schema}};
    std::vector<Column> merged = table->schema.GetColumns();

    // JOIN：每加一个表就叠一层 NLJ
    std::unique_ptr<AbstractPlanNode> plan = nullptr;
    for (const auto &join : stmt->joins) {
        TableInfo *jt = catalog_->GetTable(join.table);
        std::string jname = join.alias.empty() ? join.table : join.alias;
        scopes.emplace_back(jname, jt->schema);
        const std::vector<Column> &jcols = jt->schema.GetColumns();
        merged.insert(merged.end(), jcols.begin(), jcols.end());
        std::unique_ptr<AbstractPlanNode> left;
        if (plan != nullptr) {
            left = std::move(plan);
        } else {
            left = std::make_unique<SeqScanPlanNode>(*table, nullptr, table->schema);
        }
        JoinType type = join.type == sql::JoinClause::Type::LEFT ? JoinType::LEFT : JoinType::INNER;
        plan = std::make_unique<NestedLoopJoinPlanNode>(
            std::move(left),
            std::make_unique<SeqScanPlanNode>(*jt, nullptr, jt->schema), type,
            BindExpr(join.on.get(), scopes, 0), Schema(merged));
    }
    if (plan == nullptr) {
        plan = std::make_unique<SeqScanPlanNode>(*table, nullptr, table->schema);
    }

    // WHERE 融合（单表 → 扫描的 filter 槽；join → 顶层 NLJ 谓词合取）
    if (stmt->where != nullptr) {
        AbstractExpressionRef filter = BindExpr(stmt->where.get(), scopes, 0);
        if (plan->GetType() == PlanType::SEQ_SCAN) {
            static_cast<SeqScanPlanNode *>(plan.get())->SetFilter(std::move(filter));
        } else {
            static_cast<NestedLoopJoinPlanNode *>(plan.get())->AndPredicate(std::move(filter));
        }
    }

    // SELECT 项：* 展开 / 列选择 / 聚合收集
    bool has_agg = !stmt->group_by.empty();
    std::vector<AbstractExpressionRef> agg_exprs;
    std::vector<AbstractExpressionRef> group_exprs;
    std::vector<Column> out_cols;
    for (const auto &item : stmt->items) {
        if (item.expr->kind == Expr::Kind::STAR) {
            for (const auto &c : merged) {  // join 后 * = 全部列
                out_cols.push_back(c);
            }
            continue;
        }
        if (item.expr->kind == Expr::Kind::AGGREGATE) {
            has_agg = true;
            agg_exprs.push_back(BindExpr(item.expr.get(), scopes, 0));
            std::string name = item.alias.empty() ? item.expr->op + "()": item.alias;
            out_cols.emplace_back(name, TypeId::BIGINT);
            continue;
        }
        const Column &c = FindOutColumn(merged, item.expr.get());
        std::string name = item.alias.empty() ? c.GetName() : item.alias;
        out_cols.emplace_back(name, c.GetType(), c.GetMaxLength());
    }
    for (const auto &g : stmt->group_by) {
        group_exprs.push_back(BindExpr(g.get(), scopes, 0));
    }
    // HAVING（形如 AGG(arg) op 常量）左孩子里的聚合也要参与计算：
    // SELECT 项未引用时追加到 aggregates 尾部（仅过滤用，输出按列数截断），
    // 并记录其下标供执行器直接定位（指针比较不可靠：两处 Bind 各造新对象）。
    int having_agg_idx = -1;
    if (stmt->having != nullptr && stmt->having->left != nullptr &&
        stmt->having->left->kind == Expr::Kind::AGGREGATE) {
        having_agg_idx = static_cast<int>(agg_exprs.size());
        agg_exprs.push_back(BindExpr(stmt->having->left.get(), scopes, 0));
    }

    std::unique_ptr<AbstractPlanNode> out;
    if (has_agg) {
        AbstractExpressionRef having =
            stmt->having != nullptr ? BindExpr(stmt->having.get(), scopes, 0) : nullptr;
        out = std::make_unique<AggregationPlanNode>(std::move(plan), std::move(group_exprs),
                                                    std::move(agg_exprs), std::move(having),
                                                    Schema(out_cols), having_agg_idx);
    } else {
        out = std::move(plan);
    }

    // ORDER BY → Sort；LIMIT → Limit（TopN 合成交给 Optimizer）
    if (!stmt->order_by.empty()) {
        // join 场景：Sort 在连接输出（单流 merged）之上求值，
        // 排序键按 merged 解析列下标且槽位恒为左槽（单流模式）
        // join 后 out 的 OutputSchema 即合并模式（SeqScan=表 / NLJ=左右拼接）
        const Schema *single_stream = stmt->joins.empty() ? nullptr : &out->OutputSchema();
        std::vector<OrderByItem> obs;
        for (const auto &o : stmt->order_by) {
            obs.push_back(OrderByItem{BindExpr(o.expr.get(), scopes, 0, single_stream), o.ascending, ""});
        }
        out = std::make_unique<SortPlanNode>(std::move(out), std::move(obs));
    }
    if (stmt->limit >= 0) {
        out = std::make_unique<LimitPlanNode>(std::move(out), stmt->limit);
    }
    if (!has_agg) {
        // 投影挂**最终输出节点**：内层（扫描/排序）保持全列，
        // 排序键绑定表 schema 才能求值；顶层输出节点按投影裁剪。
        out->SetOutputSchema(Schema(out_cols));
    }
    return out;
}

// ---------------------------------------------------------------------------
// INSERT / DELETE / UPDATE（给定）
// ---------------------------------------------------------------------------
std::unique_ptr<AbstractPlanNode> Planner::PlanInsert(const sql::InsertStmt *stmt) {
    TableInfo *table = catalog_->GetTable(stmt->table);
    std::vector<std::vector<Value>> rows;
    for (const auto &ast_row : stmt->rows) {
        std::vector<Value> row;
        for (const auto &e : ast_row) {
            if (e->kind != Expr::Kind::LITERAL) {
                throw ExecutionException("INSERT values must be literals");
            }
            row.push_back(e->literal);
        }
        if (row.size() != table->schema.GetColumnCount()) {
            throw ExecutionException("INSERT row size mismatch");
        }
        rows.push_back(std::move(row));
    }
    return std::make_unique<InsertPlanNode>(*table, std::move(rows));
}

std::unique_ptr<AbstractPlanNode> Planner::PlanDelete(const sql::DeleteStmt *stmt) {
    TableInfo *table = catalog_->GetTable(stmt->table);
    AbstractExpressionRef filter =
        stmt->where != nullptr ? BindExpr(stmt->where.get(), {{stmt->table, table->schema}}, 0)
                               : nullptr;
    return std::make_unique<DeletePlanNode>(
        *table, std::make_unique<SeqScanPlanNode>(*table, std::move(filter), table->schema));
}

std::unique_ptr<AbstractPlanNode> Planner::PlanUpdate(const sql::UpdateStmt *stmt) {
    TableInfo *table = catalog_->GetTable(stmt->table);
    std::vector<std::pair<column_id_t, AbstractExpressionRef>> sets;
    for (const auto &[col_name, ast_expr] : stmt->sets) {
        const Column &col = table->schema.GetColumn(col_name);
        sets.emplace_back(col.GetColumnId(),
                          BindExpr(ast_expr.get(), {{stmt->table, table->schema}}, 0));
    }
    AbstractExpressionRef filter =
        stmt->where != nullptr ? BindExpr(stmt->where.get(), {{stmt->table, table->schema}}, 0)
                               : nullptr;
    return std::make_unique<UpdatePlanNode>(
        *table, std::move(sets),
        std::make_unique<SeqScanPlanNode>(*table, std::move(filter), table->schema));
}

}  // namespace hdb
