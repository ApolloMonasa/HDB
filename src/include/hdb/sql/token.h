#pragma once

#include <string>
#include <unordered_map>

namespace hdb::sql {

/**
 * @file token.h
 * @brief 词法单元定义（框架给定）——Lexer 的输出、Parser 的输入。
 *
 * 一个 Token = 类型 + 原文（lexeme）+ 位置（line/col）。
 * 位置信息让语法错误能报出"第几行第几列"，这在 Lab4 是体验分。
 */
enum class TokenType {
    // 关键字（大小写不敏感）
    SELECT, FROM, WHERE, INSERT, INTO, VALUES, DELETE_, UPDATE, SET,
    CREATE, TABLE, DROP, INT, BIGINT, FLOAT_, VARCHAR, BOOL,
    PRIMARY, KEY, ORDER, BY, GROUP, ASC, DESC, JOIN, ON, INNER, LEFT,
    AS, LIMIT, AND, OR, NOT, TRUE, FALSE, NULL_, COUNT, SUM, MIN, MAX, AVG,
    HAVING, EXPLAIN,

    // 字面量与标识符
    IDENTIFIER,   // 表名 / 列名
    INT_LITERAL,  // 123
    FLOAT_LITERAL,// 1.5
    STRING_LITERAL, // 'abc'
    BOOL_LITERAL, // TRUE / FALSE（词法层已折算）

    // 运算符与标点
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ,        // =
    NEQ,       // <> 或 !=
    LT, LE, GT, GE,
    COMMA, LPAREN, RPAREN, DOT, SEMI,

    END_OF_FILE,
};

/// Token 类型 -> 可读名（错误消息用）
inline const char *TokenName(TokenType t) {
    switch (t) {
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::INTO: return "INTO";
        case TokenType::VALUES: return "VALUES";
        case TokenType::DELETE_: return "DELETE";
        case TokenType::UPDATE: return "UPDATE";
        case TokenType::SET: return "SET";
        case TokenType::CREATE: return "CREATE";
        case TokenType::TABLE: return "TABLE";
        case TokenType::DROP: return "DROP";
        case TokenType::INT: return "INT";
        case TokenType::BIGINT: return "BIGINT";
        case TokenType::FLOAT_: return "FLOAT";
        case TokenType::VARCHAR: return "VARCHAR";
        case TokenType::BOOL: return "BOOL";
        case TokenType::PRIMARY: return "PRIMARY";
        case TokenType::KEY: return "KEY";
        case TokenType::ORDER: return "ORDER";
        case TokenType::BY: return "BY";
        case TokenType::GROUP: return "GROUP";
        case TokenType::ASC: return "ASC";
        case TokenType::DESC: return "DESC";
        case TokenType::JOIN: return "JOIN";
        case TokenType::ON: return "ON";
        case TokenType::INNER: return "INNER";
        case TokenType::LEFT: return "LEFT";
        case TokenType::AS: return "AS";
        case TokenType::LIMIT: return "LIMIT";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::NULL_: return "NULL";
        case TokenType::COUNT: return "COUNT";
        case TokenType::SUM: return "SUM";
        case TokenType::MIN: return "MIN";
        case TokenType::MAX: return "MAX";
        case TokenType::AVG: return "AVG";
        case TokenType::HAVING: return "HAVING";
        case TokenType::EXPLAIN: return "EXPLAIN";
        case TokenType::IDENTIFIER: return "identifier";
        case TokenType::INT_LITERAL: return "int literal";
        case TokenType::FLOAT_LITERAL: return "float literal";
        case TokenType::STRING_LITERAL: return "string literal";
        case TokenType::BOOL_LITERAL: return "bool literal";
        case TokenType::PLUS: return "+";
        case TokenType::MINUS: return "-";
        case TokenType::STAR: return "*";
        case TokenType::SLASH: return "/";
        case TokenType::PERCENT: return "%";
        case TokenType::EQ: return "=";
        case TokenType::NEQ: return "<>";
        case TokenType::LT: return "<";
        case TokenType::LE: return "<=";
        case TokenType::GT: return ">";
        case TokenType::GE: return ">=";
        case TokenType::COMMA: return ",";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::DOT: return ".";
        case TokenType::SEMI: return ";";
        case TokenType::END_OF_FILE: return "<eof>";
    }
    return "?";
}

struct Token {
    TokenType type;
    std::string lexeme;  // 原文（IDENTIFIER 保留大小写；关键字统一大写后匹配）
    int line;
    int col;
};

/// 关键字表（给定）
inline const std::unordered_map<std::string, TokenType> &KeywordTable() {
    static const std::unordered_map<std::string, TokenType> table = {
        {"SELECT", TokenType::SELECT},       {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE},         {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},           {"VALUES", TokenType::VALUES},
        {"DELETE", TokenType::DELETE_},      {"UPDATE", TokenType::UPDATE},
        {"SET", TokenType::SET},             {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},         {"DROP", TokenType::DROP},
        {"INT", TokenType::INT},             {"INTEGER", TokenType::INT},
        {"BIGINT", TokenType::BIGINT},       {"FLOAT", TokenType::FLOAT_},
        {"DOUBLE", TokenType::FLOAT_},       {"VARCHAR", TokenType::VARCHAR},
        {"BOOL", TokenType::BOOL},           {"BOOLEAN", TokenType::BOOL},
        {"PRIMARY", TokenType::PRIMARY},     {"KEY", TokenType::KEY},
        {"ORDER", TokenType::ORDER},         {"BY", TokenType::BY},
        {"GROUP", TokenType::GROUP},         {"ASC", TokenType::ASC},
        {"DESC", TokenType::DESC},           {"JOIN", TokenType::JOIN},
        {"ON", TokenType::ON},               {"INNER", TokenType::INNER},
        {"LEFT", TokenType::LEFT},           {"AS", TokenType::AS},
        {"LIMIT", TokenType::LIMIT},         {"AND", TokenType::AND},
        {"OR", TokenType::OR},               {"NOT", TokenType::NOT},
        {"TRUE", TokenType::TRUE},           {"FALSE", TokenType::FALSE},
        {"NULL", TokenType::NULL_},          {"COUNT", TokenType::COUNT},
        {"SUM", TokenType::SUM},             {"MIN", TokenType::MIN},
        {"MAX", TokenType::MAX},             {"AVG", TokenType::AVG},
        {"HAVING", TokenType::HAVING},
        {"EXPLAIN", TokenType::EXPLAIN},
    };
    return table;
}

}  // namespace hdb::sql
