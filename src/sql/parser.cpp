#include "hdb/sql/parser.h"

#include <stdexcept>

#include "hdb/common/exception.h"
#include "hdb/type/value_factory.h"

namespace hdb::sql {

// ===========================================================================
// 实现说明（先读这段再动手）：
//
// * "给定"部分：语句骨架 + 基础表达式（单层比较 + AND）。
//   只用这些功能时，parser 的基础用例就能通过。
// * "直通桩"：ParseAdditive 等中间层暂时直接转发给下一层——
//   基础表达式沿链路直通到 ParsePrimary（给定）。
//   你实现某层后，那一层开始真正消费运算符。
// * 你的任务标记为 TODO(Lab4)，集中在 docs/lab4-query.md 的"解析部分"。
// ===========================================================================

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

const Token &Parser::Peek() const { return tokens_[pos_]; }
const Token &Parser::Previous() const { return tokens_[pos_ - 1]; }
bool Parser::Check(TokenType type) const { return Peek().type == type; }

Token Parser::Advance() {
    if (!Check(TokenType::END_OF_FILE)) {
        pos_++;
    }
    return tokens_[pos_ - 1];
}

bool Parser::Match(TokenType type) {
    if (Check(type)) {
        Advance();
        return true;
    }
    return false;
}

Token Parser::Expect(TokenType type, const char *what) {
    if (Check(type)) {
        return Advance();
    }
    throw ExecutionException(std::string("parser: expected ") + what + " but got '" +
                             Peek().lexeme + "' at line " + std::to_string(Peek().line) +
                             " col " + std::to_string(Peek().col));
}

// ---------------------------------------------------------------------------
// 语句入口（给定）
// ---------------------------------------------------------------------------
std::unique_ptr<Statement> Parser::ParseStatement() {
    auto stmt = std::make_unique<Statement>();
    if (Match(TokenType::EXPLAIN)) {
        stmt->is_explain = true;  // EXPLAIN <statement>：只出计划不执行
    }
    switch (Peek().type) {
        case TokenType::SELECT: {
            stmt->type = Statement::Type::SELECT;
            auto s = ParseSelect();
            stmt->select = std::move(s->select);
            break;
        }
        case TokenType::INSERT: {
            stmt->type = Statement::Type::INSERT;
            auto s = ParseInsert();
            stmt->insert = std::move(s->insert);
            break;
        }
        case TokenType::DELETE_: {
            stmt->type = Statement::Type::DELETE_;
            auto s = ParseDelete();
            stmt->del = std::move(s->del);
            break;
        }
        case TokenType::UPDATE: {
            stmt->type = Statement::Type::UPDATE;
            auto s = ParseUpdate();
            stmt->update = std::move(s->update);
            break;
        }
        case TokenType::CREATE: {
            stmt->type = Statement::Type::CREATE;
            auto s = ParseCreate();
            stmt->create = std::move(s->create);
            break;
        }
        case TokenType::DROP: {
            stmt->type = Statement::Type::DROP;
            auto s = ParseDrop();
            stmt->drop = std::move(s->drop);
            break;
        }
        default:
            throw ExecutionException("parser: unsupported statement starting with '" +
                                     Peek().lexeme + "' at line " + std::to_string(Peek().line));
    }
    Match(TokenType::SEMI);
    if (!Check(TokenType::END_OF_FILE)) {
        throw ExecutionException("parser: unexpected trailing input '" + Peek().lexeme + "'");
    }
    return stmt;
}

// ---------------------------------------------------------------------------
// SELECT（骨架给定；JOIN/GROUP BY/ORDER BY/LIMIT 子句是 TODO）
// ---------------------------------------------------------------------------
std::unique_ptr<Statement> Parser::ParseSelect() {
    Expect(TokenType::SELECT, "SELECT");

    auto stmt = std::make_unique<SelectStmt>();
    stmt->items.push_back(ParseSelectItem());
    while (Match(TokenType::COMMA)) {
        stmt->items.push_back(ParseSelectItem());
    }

    Expect(TokenType::FROM, "FROM");
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    if (Match(TokenType::AS)) {
        stmt->alias = Expect(TokenType::IDENTIFIER, "table alias").lexeme;
    } else if (Check(TokenType::IDENTIFIER)) {
        stmt->alias = Advance().lexeme;  // 隐式别名
    }

    if (Check(TokenType::INNER) || Check(TokenType::LEFT) || Check(TokenType::JOIN)) {
        stmt->joins = ParseJoinClauses();  // TODO(Lab4)
    }

    if (Match(TokenType::WHERE)) {
        stmt->where = ParseExpression();
    }

    if (Check(TokenType::GROUP)) {
        ParseGroupBy(stmt.get());  // TODO(Lab4)
    }

    if (Check(TokenType::ORDER)) {
        ParseOrderBy(stmt.get());  // TODO(Lab4)
    }

    if (Check(TokenType::LIMIT)) {
        ParseLimit(stmt.get());  // TODO(Lab4)
    }

    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::SELECT;
    s->select = std::move(stmt);
    return s;
}

SelectItem Parser::ParseSelectItem() {
    SelectItem item;
    item.expr = ParseExpression();
    if (Match(TokenType::AS)) {
        item.alias = Expect(TokenType::IDENTIFIER, "alias").lexeme;
    }
    return item;
}

std::unique_ptr<Expr> Parser::ParseColumnRef() {
    // 两种形态：a 或 t.a
    Token first = Expect(TokenType::IDENTIFIER, "column name");
    if (Match(TokenType::DOT)) {
        Token col = Expect(TokenType::IDENTIFIER, "column name after '.'");
        return Expr::Column(first.lexeme, col.lexeme);
    }
    return Expr::Column("", first.lexeme);
}

// ---------------------------------------------------------------------------
// INSERT / DELETE / UPDATE / CREATE / DROP（全部给定）
// ---------------------------------------------------------------------------
std::unique_ptr<Statement> Parser::ParseInsert() {
    Expect(TokenType::INSERT, "INSERT");
    Expect(TokenType::INTO, "INTO");
    auto stmt = std::make_unique<InsertStmt>();
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    Expect(TokenType::VALUES, "VALUES");

    do {
        Expect(TokenType::LPAREN, "'('");
        std::vector<std::unique_ptr<Expr>> row;
        row.push_back(ParseExpression());
        while (Match(TokenType::COMMA)) {
            row.push_back(ParseExpression());
        }
        Expect(TokenType::RPAREN, "')'");
        stmt->rows.push_back(std::move(row));
    } while (Match(TokenType::COMMA));

    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::INSERT;
    s->insert = std::move(stmt);
    return s;
}

std::unique_ptr<Statement> Parser::ParseDelete() {
    Expect(TokenType::DELETE_, "DELETE");
    Expect(TokenType::FROM, "FROM");
    auto stmt = std::make_unique<DeleteStmt>();
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    if (Match(TokenType::WHERE)) {
        stmt->where = ParseExpression();
    }
    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::DELETE_;
    s->del = std::move(stmt);
    return s;
}

std::unique_ptr<Statement> Parser::ParseUpdate() {
    Expect(TokenType::UPDATE, "UPDATE");
    auto stmt = std::make_unique<UpdateStmt>();
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    Expect(TokenType::SET, "SET");
    do {
        std::string col = Expect(TokenType::IDENTIFIER, "column name").lexeme;
        Expect(TokenType::EQ, "'='");
        stmt->sets.emplace_back(col, ParseExpression());
    } while (Match(TokenType::COMMA));
    if (Match(TokenType::WHERE)) {
        stmt->where = ParseExpression();
    }
    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::UPDATE;
    s->update = std::move(stmt);
    return s;
}

std::unique_ptr<Statement> Parser::ParseCreate() {
    Expect(TokenType::CREATE, "CREATE");
    Expect(TokenType::TABLE, "TABLE");
    auto stmt = std::make_unique<CreateStmt>();
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    Expect(TokenType::LPAREN, "'('");
    do {
        ColumnDef def;
        def.name = Expect(TokenType::IDENTIFIER, "column name").lexeme;
        switch (Advance().type) {
            case TokenType::INT: def.type = TypeId::INTEGER; break;
            case TokenType::BIGINT: def.type = TypeId::BIGINT; break;
            case TokenType::FLOAT_: def.type = TypeId::FLOAT; break;
            case TokenType::BOOL: def.type = TypeId::BOOLEAN; break;
            case TokenType::VARCHAR: {
                def.type = TypeId::VARCHAR;
                Expect(TokenType::LPAREN, "'('");
                def.varchar_len =
                    static_cast<uint32_t>(std::stoi(Expect(TokenType::INT_LITERAL, "length").lexeme));
                Expect(TokenType::RPAREN, "')'");
                break;
            }
            default:
                throw ExecutionException("parser: unknown column type for '" + def.name + "'");
        }
        if (Check(TokenType::PRIMARY)) {
            Advance();
            Expect(TokenType::KEY, "KEY");
            def.primary_key = true;
        }
        stmt->columns.push_back(std::move(def));
    } while (Match(TokenType::COMMA));
    Expect(TokenType::RPAREN, "')'");

    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::CREATE;
    s->create = std::move(stmt);
    return s;
}

std::unique_ptr<Statement> Parser::ParseDrop() {
    Expect(TokenType::DROP, "DROP");
    Expect(TokenType::TABLE, "TABLE");
    auto stmt = std::make_unique<DropStmt>();
    stmt->table = Expect(TokenType::IDENTIFIER, "table name").lexeme;
    auto s = std::make_unique<Statement>();
    s->type = Statement::Type::DROP;
    s->drop = std::move(stmt);
    return s;
}

// ---------------------------------------------------------------------------
// 表达式层级（给定：AND 与单层比较；直通桩：中间层；TODO：完备化）
// ---------------------------------------------------------------------------
std::unique_ptr<Expr> Parser::ParseExpression() {
    // TODO(Lab4): OR 层——expr ::= and_expr (OR and_expr)*
    return ParseAndExpression();
}

std::unique_ptr<Expr> Parser::ParseAndExpression() {
    auto expr = ParseNotExpression();
    while (Check(TokenType::AND)) {
        Advance();
        expr = Expr::Bin(Expr::Kind::LOGIC_AND, "AND", std::move(expr), ParseNotExpression());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseNotExpression() {
    // TODO(Lab4): NOT not_expr | comparison（直通桩：先透传到比较层）
    return ParseComparison();
}

std::unique_ptr<Expr> Parser::ParseComparison() {
    // 给定：addsub (('='|'<>'|'<'|'<='|'>'|'>=') addsub)?
    auto expr = ParseAdditive();
    const char *op = nullptr;
    if (Match(TokenType::EQ)) {
        op = "=";
    } else if (Match(TokenType::NEQ)) {
        op = "<>";
    } else if (Match(TokenType::LT)) {
        op = "<";
    } else if (Match(TokenType::LE)) {
        op = "<=";
    } else if (Match(TokenType::GT)) {
        op = ">";
    } else if (Match(TokenType::GE)) {
        op = ">=";
    }
    if (op != nullptr) {
        return Expr::Bin(Expr::Kind::BINARY_OP, op, std::move(expr), ParseAdditive());
    }
    return expr;
}

std::unique_ptr<Expr> Parser::ParseAdditive() {
    // TODO(Lab4): (('+'|'-') muldiv)* 左结合
    return ParseMultiplicative();
}

std::unique_ptr<Expr> Parser::ParseMultiplicative() {
    // TODO(Lab4): (('*'|'/'|'%') unary)* 左结合
    return ParseUnary();
}

std::unique_ptr<Expr> Parser::ParseUnary() {
    // TODO(Lab4): '-' unary（负号）
    return ParsePrimary();
}

std::unique_ptr<Expr> Parser::ParsePrimary() {
    // 给定：字面量 | 列引用 | '(' expr ')'
    if (Check(TokenType::INT_LITERAL)) {
        return Expr::Lit(ValueFactory::CreateInteger(
            std::stoi(Advance().lexeme)));  // NOLINT：超 int 范围属于用户输入错误
    }
    if (Check(TokenType::FLOAT_LITERAL)) {
        return Expr::Lit(ValueFactory::CreateFloat(std::stod(Advance().lexeme)));
    }
    if (Check(TokenType::STRING_LITERAL)) {
        return Expr::Lit(ValueFactory::CreateVarchar(Advance().lexeme));
    }
    if (Check(TokenType::TRUE) || Check(TokenType::FALSE)) {
        return Expr::Lit(ValueFactory::CreateBoolean(Advance().type == TokenType::TRUE));
    }
    if (Match(TokenType::NULL_)) {
        return Expr::Lit(ValueFactory::CreateNull());
    }
    if (Match(TokenType::LPAREN)) {
        auto expr = ParseExpression();
        Expect(TokenType::RPAREN, "')'");
        return expr;
    }
    // 聚合函数 COUNT/SUM/MIN/MAX/AVG
    switch (Peek().type) {
        case TokenType::COUNT:
        case TokenType::SUM:
        case TokenType::MIN:
        case TokenType::MAX:
        case TokenType::AVG:
            return ParseAggregateCall();  // TODO(Lab4)
        default:
            break;
    }
    if (Check(TokenType::STAR)) {
        Advance();
        return Expr::Star();
    }
    if (Check(TokenType::IDENTIFIER)) {
        return ParseColumnRef();
    }
    throw ExecutionException("parser: unexpected token '" + Peek().lexeme + "' in expression");
}

std::unique_ptr<Expr> Parser::ParseAggregateCall() {
    // TODO(Lab4): FUNC '(' (expr | '*') ')'
    (void)0;
    throw NotImplementedException("Lab4: implement Parser::ParseAggregateCall (docs/lab4-query.md)");
}

// ---------------------------------------------------------------------------
// SELECT 高级子句（TODO(Lab4)，只在这些关键字出现时被调用）
// ---------------------------------------------------------------------------
std::vector<JoinClause> Parser::ParseJoinClauses() {
    throw NotImplementedException("Lab4: implement Parser::ParseJoinClauses");
}

void Parser::ParseGroupBy(SelectStmt *stmt) {
    (void)stmt;
    throw NotImplementedException("Lab4: implement Parser::ParseGroupBy");
}

void Parser::ParseOrderBy(SelectStmt *stmt) {
    (void)stmt;
    throw NotImplementedException("Lab4: implement Parser::ParseOrderBy");
}

void Parser::ParseLimit(SelectStmt *stmt) {
    (void)stmt;
    throw NotImplementedException("Lab4: implement Parser::ParseLimit");
}

}  // namespace hdb::sql
