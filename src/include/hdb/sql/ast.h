#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hdb/storage/table/column.h"
#include "hdb/type/value.h"

namespace hdb::sql {

/**
 * @file ast.h
 * @brief 语法树节点（框架给定）——Parser 的输出。
 *
 * AST 只描述"语法结构"，不做任何名字解析：
 * 它口中的表名列名都还是字符串，等 Planner 拿 Catalog 翻译。
 * 所有节点用 unique_ptr 串接，整棵树无环、所有权清晰。
 */

// ---------------------------------------------------------------------------
// 表达式
// ---------------------------------------------------------------------------
struct Expr {
    enum class Kind {
        COLUMN_REF,  // a / t.a
        LITERAL,     // 1, 'x', 2.5, TRUE, NULL
        BINARY_OP,   // 算术 + - * / % 与比较 = <> < <= > >=
        LOGIC_AND,
        LOGIC_OR,
        LOGIC_NOT,
        STAR,        // SELECT * 的 *
        AGGREGATE,   // COUNT/SUM/MIN/MAX/AVG(expr)
    };

    Kind kind;
    // COLUMN_REF
    std::string table;  // 可空（限定名 t.a 的 t）
    std::string column;
    // LITERAL
    Value literal;
    // 运算符原文（"+", "<>", "AND"...）
    std::string op;
    // AGGREGATE：函数名
    // BINARY / NOT 的操作数
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    static std::unique_ptr<Expr> Column(std::string table, std::string col) {
        auto e = std::make_unique<Expr>();
        e->kind = Kind::COLUMN_REF;
        e->table = std::move(table);
        e->column = std::move(col);
        return e;
    }
    static std::unique_ptr<Expr> Lit(Value v) {
        auto e = std::make_unique<Expr>();
        e->kind = Kind::LITERAL;
        e->literal = std::move(v);
        return e;
    }
    static std::unique_ptr<Expr> Star() {
        auto e = std::make_unique<Expr>();
        e->kind = Kind::STAR;
        return e;
    }
    static std::unique_ptr<Expr> Bin(Kind kind, std::string op_str, std::unique_ptr<Expr> l,
                                     std::unique_ptr<Expr> r) {
        auto e = std::make_unique<Expr>();
        e->kind = kind;
        e->op = std::move(op_str);
        e->left = std::move(l);
        e->right = std::move(r);
        return e;
    }
    static std::unique_ptr<Expr> Not(std::unique_ptr<Expr> child) {
        auto e = std::make_unique<Expr>();
        e->kind = Kind::LOGIC_NOT;
        e->op = "NOT";
        e->left = std::move(child);
        return e;
    }
    static std::unique_ptr<Expr> Agg(std::string func, std::unique_ptr<Expr> child) {
        auto e = std::make_unique<Expr>();
        e->kind = Kind::AGGREGATE;
        e->op = std::move(func);  // COUNT / SUM / MIN / MAX / AVG
        e->left = std::move(child);
        return e;
    }
};

// ---------------------------------------------------------------------------
// 语句
// ---------------------------------------------------------------------------
struct SelectItem {
    std::unique_ptr<Expr> expr;  // 列引用 / * / 聚合 / 表达式
    std::string alias;           // AS 别名（可空）
};

struct JoinClause {
    enum class Type { INNER, LEFT };
    Type type = Type::INNER;
    std::string table;
    std::string alias;             // 可空
    std::unique_ptr<Expr> on;      // ON 条件
};

struct OrderItem {
    std::unique_ptr<Expr> expr;
    bool ascending = true;
};

struct Statement;

/** SELECT ... */
struct SelectStmt {
    std::vector<SelectItem> items;
    std::string table;                 // FROM 主表
    std::string alias;                 // 主表别名
    std::vector<JoinClause> joins;     // 0..n 个连接
    std::unique_ptr<Expr> where;       // 可空
    std::vector<std::unique_ptr<Expr>> group_by;  // 可空
    std::unique_ptr<Expr> having;      // 可空
    std::vector<OrderItem> order_by;   // 可空
    int limit = -1;                    // -1 = 无 LIMIT
};

/** INSERT INTO t VALUES (...), (...) */
struct InsertStmt {
    std::string table;
    std::vector<std::vector<std::unique_ptr<Expr>>> rows;  // 每行一组字面量表达式
};

/** DELETE FROM t [WHERE ...] */
struct DeleteStmt {
    std::string table;
    std::unique_ptr<Expr> where;
};

/** UPDATE t SET c = expr, ... [WHERE ...] */
struct UpdateStmt {
    std::string table;
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> sets;
    std::unique_ptr<Expr> where;
};

/** 建表里的一列定义 */
struct ColumnDef {
    std::string name;
    TypeId type = TypeId::INVALID;
    uint32_t varchar_len = 0;
    bool primary_key = false;  // PRIMARY KEY 紧跟该列定义
};

/** CREATE TABLE t (a INT PRIMARY KEY, b VARCHAR(20), ...) */
struct CreateStmt {
    std::string table;
    std::vector<ColumnDef> columns;
};

/** DROP TABLE t */
struct DropStmt {
    std::string table;
};

/** 语句的统一外壳 */
struct Statement {
    enum class Type { SELECT, INSERT, DELETE_, UPDATE, CREATE, DROP };
    Type type;
    bool is_explain{false};  // EXPLAIN 前缀：只打印计划树不执行
    std::unique_ptr<SelectStmt> select;
    std::unique_ptr<InsertStmt> insert;
    std::unique_ptr<DeleteStmt> del;
    std::unique_ptr<UpdateStmt> update;
    std::unique_ptr<CreateStmt> create;
    std::unique_ptr<DropStmt> drop;
};

}  // namespace hdb::sql
