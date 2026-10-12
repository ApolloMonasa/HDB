#pragma once

#include <string>
#include <vector>

#include "hdb/sql/token.h"

namespace hdb::sql {

class ParseException;

/**
 * @brief 词法分析器（框架给定）——SQL 文本 → Token 流。
 *
 * 处理的词法规则：
 *   * 空白与换行（含 -- 行注释）跳过，但换行累计行号；
 *   * 标识符：[A-Za-z_][A-Za-z0-9_]*，查关键字表（大小写不敏感）；
 *   * 数字：整数 / 小数（不支持科学计数法，够用）；
 *   * 字符串：单引号包裹，'' 转义一个单引号；
 *   * 运算符：>= <= <> != = < > + - * / % , ( ) . ;
 *
 * 输出以 EOF Token 收尾。
 */
class Lexer {
  public:
    explicit Lexer(std::string sql);

    /** 扫描出全部 Token（一次性；HDB 语句量级小，无需流式） */
    std::vector<Token> Tokenize();

  private:
    char Peek() const;
    char PeekNext() const;
    char Advance();
    bool AtEnd() const;
    void SkipWhitespaceAndComments();
    Token MakeToken(TokenType type, std::string lexeme) const;
    Token ScanIdentifier();
    Token ScanNumber();
    Token ScanString();

    std::string sql_;
    size_t pos_{0};
    int line_{1};
    int col_{1};
};

}  // namespace hdb::sql
