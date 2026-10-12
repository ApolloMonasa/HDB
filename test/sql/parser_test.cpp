/**
 * test/sql/parser_test.cpp —— Lab 4（解析部分）判卷用例
 *
 * 前半部分测**框架给定**的词法/语法骨架，开局即绿；
 * 后半部分（ORDER BY/GROUP BY/JOIN/LIMIT/聚合/算术表达式）依赖你的
 * TODO 实现，做完后转绿。
 */
#include <algorithm>

#include "gtest/gtest.h"
#include "hdb/common/exception.h"
#include "hdb/sql/lexer.h"
#include "hdb/sql/parser.h"

namespace hdb::test {

namespace {
using namespace hdb::sql;

std::unique_ptr<Statement> ParseOne(const std::string &sql) {
    Lexer lexer(sql);
    Parser parser(lexer.Tokenize());
    return parser.ParseStatement();
}
}  // namespace

// ===========================================================================
// 给定部分：开局即绿
// ===========================================================================
TEST(ParserTest, LexBasics) {
    Lexer lexer("SELECT a, b FROM t WHERE x >= 1.5 AND y <> 'o''k'");
    auto tokens = lexer.Tokenize();
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "a");
    EXPECT_EQ(tokens[4].type, TokenType::FROM);
    // 1.5 浮点
    auto it = std::find_if(tokens.begin(), tokens.end(), [](const Token &t) {
        return t.type == TokenType::FLOAT_LITERAL;
    });
    ASSERT_NE(it, tokens.end());
    EXPECT_EQ(it->lexeme, "1.5");
    // 'o''k' 转义成 o'k
    auto s = std::find_if(tokens.begin(), tokens.end(), [](const Token &t) {
        return t.type == TokenType::STRING_LITERAL;
    });
    ASSERT_NE(s, tokens.end());
    EXPECT_EQ(s->lexeme, "o'k");
    EXPECT_EQ(tokens.back().type, TokenType::END_OF_FILE);
}

TEST(ParserTest, LexKeywordCaseInsensitive) {
    Lexer lexer("select From WHERE");
    auto tokens = lexer.Tokenize();
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::FROM);
    EXPECT_EQ(tokens[2].type, TokenType::WHERE);
}

TEST(ParserTest, LexCommentAndNewline) {
    Lexer lexer("SELECT -- this is a comment\na");
    auto tokens = lexer.Tokenize();
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "a");
    EXPECT_EQ(tokens[1].line, 2);  // 行号正确累计
}

TEST(ParserTest, ParseCreate) {
    auto stmt = ParseOne("CREATE TABLE student (id INT PRIMARY KEY, name VARCHAR(20), gpa FLOAT)");
    ASSERT_EQ(stmt->type, Statement::Type::CREATE);
    ASSERT_EQ(stmt->create->columns.size(), 3U);
    EXPECT_EQ(stmt->create->table, "student");
    EXPECT_EQ(stmt->create->columns[0].name, "id");
    EXPECT_EQ(stmt->create->columns[0].type, TypeId::INTEGER);
    EXPECT_TRUE(stmt->create->columns[0].primary_key);
    EXPECT_EQ(stmt->create->columns[1].type, TypeId::VARCHAR);
    EXPECT_EQ(stmt->create->columns[1].varchar_len, 20U);
    EXPECT_EQ(stmt->create->columns[2].type, TypeId::FLOAT);
}

TEST(ParserTest, ParseInsert) {
    auto stmt = ParseOne("INSERT INTO t VALUES (1, 'a'), (2, 'b')");
    ASSERT_EQ(stmt->type, Statement::Type::INSERT);
    ASSERT_EQ(stmt->insert->rows.size(), 2U);
    ASSERT_EQ(stmt->insert->rows[0].size(), 2U);
    EXPECT_EQ(stmt->insert->rows[1][0]->literal.GetAsInteger(), 2);
    EXPECT_EQ(stmt->insert->rows[1][1]->literal.GetAsVarchar(), "b");
}

TEST(ParserTest, ParseDeleteUpdate) {
    auto del = ParseOne("DELETE FROM t WHERE a > 5");
    ASSERT_EQ(del->type, Statement::Type::DELETE_);
    ASSERT_NE(del->del->where, nullptr);
    EXPECT_EQ(del->del->where->kind, sql::Expr::Kind::BINARY_OP);

    auto upd = ParseOne("UPDATE t SET b = 'x', c = 3 WHERE a = 1");
    ASSERT_EQ(upd->type, Statement::Type::UPDATE);
    ASSERT_EQ(upd->update->sets.size(), 2U);
    EXPECT_EQ(upd->update->sets[0].first, "b");
}

TEST(ParserTest, ParseSelectBasics) {
    auto stmt = ParseOne("SELECT id, name FROM student WHERE id > 3 AND gpa <= 3.5");
    ASSERT_EQ(stmt->type, Statement::Type::SELECT);
    ASSERT_EQ(stmt->select->items.size(), 2U);
    EXPECT_EQ(stmt->select->table, "student");
    ASSERT_NE(stmt->select->where, nullptr);
    // WHERE 是 AND 链
    EXPECT_EQ(stmt->select->where->kind, sql::Expr::Kind::LOGIC_AND);
    // 左右各是一个比较
    EXPECT_EQ(stmt->select->where->left->kind, sql::Expr::Kind::BINARY_OP);
    EXPECT_EQ(stmt->select->where->right->kind, sql::Expr::Kind::BINARY_OP);
}

TEST(ParserTest, ParseSelectStarAndAlias) {
    auto stmt = ParseOne("SELECT * FROM student AS s");
    EXPECT_EQ(stmt->select->items[0].expr->kind, sql::Expr::Kind::STAR);
    EXPECT_EQ(stmt->select->alias, "s");
}

TEST(ParserTest, SyntaxErrorReportsLocation) {
    EXPECT_THROW(ParseOne("SELECT FROM t"), ExecutionException);
    EXPECT_THROW(ParseOne("SELECT a FROM"), ExecutionException);
    EXPECT_THROW(ParseOne("CREATE TABLE t (a STRANGE)"), ExecutionException);
}

// ===========================================================================
// 你的填空部分：做完 Lab4 解析任务后转绿
// ===========================================================================
TEST(ParserTest, ParseArithmeticPrecedence) {
    // a + b * 2 - c：先乘后加减、左结合
    auto stmt = ParseOne("SELECT a FROM t WHERE a + b * 2 - c > 0");
    ASSERT_NE(stmt->select->where, nullptr);
    // 顶层的左操作数应是减法（(- (+ a (* b 2)) c)）
    EXPECT_EQ(stmt->select->where->left->op, "-");
    EXPECT_EQ(stmt->select->where->left->left->op, "+");
    EXPECT_EQ(stmt->select->where->left->left->right->op, "*");
}

TEST(ParserTest, ParseOrNotAndParentheses) {
    auto stmt = ParseOne("SELECT a FROM t WHERE NOT (a = 1 OR b = 2) AND c = 3");
    ASSERT_NE(stmt->select->where, nullptr);
    EXPECT_EQ(stmt->select->where->kind, sql::Expr::Kind::LOGIC_AND);
    EXPECT_EQ(stmt->select->where->left->kind, sql::Expr::Kind::LOGIC_NOT);
    EXPECT_EQ(stmt->select->where->left->left->kind, sql::Expr::Kind::LOGIC_OR);
}

TEST(ParserTest, ParseOrderBy) {
    auto stmt = ParseOne("SELECT a, b FROM t ORDER BY a DESC, b");
    ASSERT_EQ(stmt->select->order_by.size(), 2U);
    EXPECT_FALSE(stmt->select->order_by[0].ascending);
    EXPECT_TRUE(stmt->select->order_by[1].ascending);
}

TEST(ParserTest, ParseLimit) {
    auto stmt = ParseOne("SELECT a FROM t LIMIT 10");
    EXPECT_EQ(stmt->select->limit, 10);
}

TEST(ParserTest, ParseGroupByHaving) {
    auto stmt = ParseOne("SELECT dept, COUNT(id) FROM t GROUP BY dept HAVING COUNT(id) > 2");
    ASSERT_EQ(stmt->select->group_by.size(), 1U);
    ASSERT_NE(stmt->select->having, nullptr);
    // SELECT 项里有聚合
    EXPECT_EQ(stmt->select->items[1].expr->kind, sql::Expr::Kind::AGGREGATE);
    EXPECT_EQ(stmt->select->items[1].expr->op, "COUNT");
}

TEST(ParserTest, ParseJoin) {
    auto stmt = ParseOne(
        "SELECT t1.a, t2.b FROM t1 INNER JOIN t2 ON t1.id = t2.id LEFT JOIN t3 ON t2.x = t3.x");
    ASSERT_EQ(stmt->select->joins.size(), 2U);
    EXPECT_EQ(stmt->select->joins[0].type, sql::JoinClause::Type::INNER);
    EXPECT_EQ(stmt->select->joins[1].type, sql::JoinClause::Type::LEFT);
    EXPECT_EQ(stmt->select->joins[0].on->kind, sql::Expr::Kind::BINARY_OP);
    // 限定列名 t1.id
    EXPECT_EQ(stmt->select->joins[0].on->left->table, "t1");
    EXPECT_EQ(stmt->select->joins[0].on->left->column, "id");
}

TEST(ParserTest, ParseAggregateCall) {
    auto stmt = ParseOne("SELECT SUM(gpa), MAX(age), COUNT(*) FROM t");
    ASSERT_EQ(stmt->select->items.size(), 3U);
    EXPECT_EQ(stmt->select->items[0].expr->kind, sql::Expr::Kind::AGGREGATE);
    EXPECT_EQ(stmt->select->items[0].expr->op, "SUM");
    EXPECT_EQ(stmt->select->items[2].expr->left->kind, sql::Expr::Kind::STAR);
}

}  // namespace hdb::test
