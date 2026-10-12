#pragma once

#include <memory>
#include <vector>

#include "hdb/sql/ast.h"
#include "hdb/sql/token.h"

namespace hdb::sql {

/**
 * @file parser.h
 * @brief Lab 4（解析部分）：递归下降语法分析器。
 *
 * 每个 ParseXxx 对应一条（组）产生式，"看到什么 Token 就走哪条路"。
 * 框架已给定：语句骨架（SELECT/INSERT/DELETE/UPDATE/CREATE/DROP 的
 * 主体结构）与基础表达式（单层比较 + AND）。
 * 你的填空：表达式完备化、ORDER BY / GROUP BY / HAVING / JOIN 子句。
 *
 * 文法速览（EBNF；『★』= 学生实现）：
 *   statement  ::= select | insert | delete | update | create | drop
 *   select     ::= SELECT item (',' item)* FROM table [joins★]
 *                  [WHERE expr] [GROUP BY expr (',' expr)*★] [HAVING expr★]
 *                  [ORDER BY item [ASC|DESC] (',' ...)*★] [LIMIT num★]
 *   expr       ::= or_expr★                      -- 优先级：OR 最低
 *   or_expr    ::= and_expr (OR and_expr)*★
 *   and_expr   ::= not_expr (AND not_expr)*       -- 给定
 *   not_expr   ::= NOT not_expr | comparison★
 *   comparison ::= addsub (('='|'<>'|'<'|'<='|'>'|'>=') addsub)?   -- 给定
 *   addsub     ::= muldiv (('+'|'-') muldiv)*★   -- 算术优先于比较
 *   muldiv     ::= unary (('*'|'/'|'%') unary)*★
 *   unary      ::= '-' unary | primary★
 *   primary    ::= literal | column | '(' expr ')' | agg_call★
 */
class Parser {
  public:
    explicit Parser(std::vector<Token> tokens);

    /** 解析出一条语句（不含结尾分号也容忍） */
    std::unique_ptr<Statement> ParseStatement();

  private:
    // ---- 游标与基础动作（给定） ----
    const Token &Peek() const;
    const Token &Previous() const;
    bool Check(TokenType type) const;
    Token Advance();
    bool Match(TokenType type);
    Token Expect(TokenType type, const char *what);

    // ---- 语句（给定骨架） ----
    std::unique_ptr<Statement> ParseSelect();
    std::unique_ptr<Statement> ParseInsert();
    std::unique_ptr<Statement> ParseDelete();
    std::unique_ptr<Statement> ParseUpdate();
    std::unique_ptr<Statement> ParseCreate();
    std::unique_ptr<Statement> ParseDrop();

    // ---- 表达式 ----
    std::unique_ptr<Expr> ParseExpression();          // TODO(Lab4)
    std::unique_ptr<Expr> ParseAndExpression();       // 给定
    std::unique_ptr<Expr> ParseNotExpression();       // TODO(Lab4)：NOT 层
    std::unique_ptr<Expr> ParseComparison();          // 给定
    std::unique_ptr<Expr> ParseAdditive();            // TODO(Lab4)
    std::unique_ptr<Expr> ParseMultiplicative();      // TODO(Lab4)
    std::unique_ptr<Expr> ParseUnary();               // TODO(Lab4)
    std::unique_ptr<Expr> ParsePrimary();             // TODO(Lab4)
    std::unique_ptr<Expr> ParseAggregateCall();       // TODO(Lab4)

    // ---- SELECT 子句 ----
    SelectItem ParseSelectItem();                     // 给定
    std::unique_ptr<Expr> ParseColumnRef();           // 给定

    // ---- SELECT 高级子句（TODO(Lab4)：检测到相应关键字时被调用） ----
    std::vector<JoinClause> ParseJoinClauses();       // JOIN ... ON ...
    void ParseGroupBy(SelectStmt *stmt);              // GROUP BY ... [HAVING ...]
    void ParseOrderBy(SelectStmt *stmt);              // ORDER BY ... [ASC|DESC]
    void ParseLimit(SelectStmt *stmt);                // LIMIT n

    std::vector<Token> tokens_;
    size_t pos_{0};
};

}  // namespace hdb::sql
