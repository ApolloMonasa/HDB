#include "hdb/sql/lexer.h"

#include <cctype>

#include "hdb/common/exception.h"

namespace hdb::sql {

Lexer::Lexer(std::string sql) : sql_(std::move(sql)) {}

char Lexer::Peek() const { return pos_ < sql_.size() ? sql_[pos_] : '\0'; }
char Lexer::PeekNext() const { return pos_ + 1 < sql_.size() ? sql_[pos_ + 1] : '\0'; }
bool Lexer::AtEnd() const { return pos_ >= sql_.size(); }

char Lexer::Advance() {
    char c = sql_[pos_++];
    if (c == '\n') {
        line_++;
        col_ = 1;
    } else {
        col_++;
    }
    return c;
}

void Lexer::SkipWhitespaceAndComments() {
    while (!AtEnd()) {
        char c = Peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            Advance();
            continue;
        }
        if (c == '-' && PeekNext() == '-') {  // 行注释
            while (!AtEnd() && Peek() != '\n') {
                Advance();
            }
            continue;
        }
        break;
    }
}

Token Lexer::MakeToken(TokenType type, std::string lexeme) const {
    return Token{type, std::move(lexeme), line_, col_};
}

Token Lexer::ScanIdentifier() {
    std::string word;
    int line = line_, col = col_;
    while (!AtEnd() && (std::isalnum(static_cast<unsigned char>(Peek())) || Peek() == '_')) {
        word += Advance();
    }
    // 关键字大小写不敏感：统一大写后查表
    std::string upper;
    upper.reserve(word.size());
    for (char c : word) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    auto it = KeywordTable().find(upper);
    if (it != KeywordTable().end()) {
        return Token{it->second, upper, line, col};
    }
    return Token{TokenType::IDENTIFIER, word, line, col};
}

Token Lexer::ScanNumber() {
    int line = line_, col = col_;
    std::string num;
    while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
        num += Advance();
    }
    if (Peek() == '.' && std::isdigit(static_cast<unsigned char>(PeekNext()))) {
        num += Advance();  // '.'
        while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) {
            num += Advance();
        }
        return Token{TokenType::FLOAT_LITERAL, num, line, col};
    }
    return Token{TokenType::INT_LITERAL, num, line, col};
}

Token Lexer::ScanString() {
    int line = line_, col = col_;
    Advance();  // 吃掉开头单引号
    std::string str;
    while (!AtEnd()) {
        char c = Advance();
        if (c == '\'') {
            if (Peek() == '\'') {  // '' 转义
                str += Advance();
                continue;
            }
            return Token{TokenType::STRING_LITERAL, str, line, col};
        }
        str += c;
    }
    throw ExecutionException("lexer: unterminated string at line " + std::to_string(line));
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;
    while (true) {
        SkipWhitespaceAndComments();
        if (AtEnd()) {
            break;
        }
        char c = Peek();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(ScanIdentifier());
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(ScanNumber());
            continue;
        }
        if (c == '\'') {
            tokens.push_back(ScanString());
            continue;
        }
        int line = line_, col = col_;
        Advance();
        switch (c) {
            case '+': tokens.push_back(Token{TokenType::PLUS, "+", line, col}); break;
            case '-': tokens.push_back(Token{TokenType::MINUS, "-", line, col}); break;
            case '*': tokens.push_back(Token{TokenType::STAR, "*", line, col}); break;
            case '/': tokens.push_back(Token{TokenType::SLASH, "/", line, col}); break;
            case '%': tokens.push_back(Token{TokenType::PERCENT, "%", line, col}); break;
            case '=': tokens.push_back(Token{TokenType::EQ, "=", line, col}); break;
            case ',': tokens.push_back(Token{TokenType::COMMA, ",", line, col}); break;
            case '(': tokens.push_back(Token{TokenType::LPAREN, "(", line, col}); break;
            case ')': tokens.push_back(Token{TokenType::RPAREN, ")", line, col}); break;
            case '.': tokens.push_back(Token{TokenType::DOT, ".", line, col}); break;
            case ';': tokens.push_back(Token{TokenType::SEMI, ";", line, col}); break;
            case '<':
                if (Peek() == '=') {
                    Advance();
                    tokens.push_back(Token{TokenType::LE, "<=", line, col});
                } else if (Peek() == '>') {
                    Advance();
                    tokens.push_back(Token{TokenType::NEQ, "<>", line, col});
                } else {
                    tokens.push_back(Token{TokenType::LT, "<", line, col});
                }
                break;
            case '>':
                if (Peek() == '=') {
                    Advance();
                    tokens.push_back(Token{TokenType::GE, ">=", line, col});
                } else {
                    tokens.push_back(Token{TokenType::GT, ">", line, col});
                }
                break;
            case '!':
                if (Peek() == '=') {
                    Advance();
                    tokens.push_back(Token{TokenType::NEQ, "!=", line, col});
                } else {
                    throw ExecutionException("lexer: unexpected '!' at line " + std::to_string(line));
                }
                break;
            default:
                throw ExecutionException(std::string("lexer: unexpected character '") + c +
                                         "' at line " + std::to_string(line));
        }
    }
    tokens.push_back(Token{TokenType::END_OF_FILE, "", line_, col_});
    return tokens;
}

}  // namespace hdb::sql
