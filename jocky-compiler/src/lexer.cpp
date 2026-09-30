#include "jocky/lexer.h"
#include <cctype>
#include <unordered_map>

namespace jocky {

Lexer::Lexer(const std::string& input) : input_(input) {}

char Lexer::current() const {
    if (position_ >= input_.size()) return '\0';
    return input_[position_];
}

char Lexer::peek(size_t offset) const {
    size_t pos = position_ + offset;
    if (pos >= input_.size()) return '\0';
    return input_[pos];
}

void Lexer::advance() {
    if (position_ < input_.size()) {
        if (input_[position_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        position_++;
    }
}

void Lexer::skipWhitespace() {
    while (std::isspace(static_cast<unsigned char>(current()))) {
        advance();
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, column_);
}

bool Lexer::isKeyword(const std::string& str, TokenType& outType) const {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"query", TokenType::KW_QUERY},
        {"let", TokenType::KW_LET},
        {"if", TokenType::KW_IF},
        {"else", TokenType::KW_ELSE},
        {"while", TokenType::KW_WHILE},
        {"for", TokenType::KW_FOR},
        {"in", TokenType::KW_IN},
        {"return", TokenType::KW_RETURN},
        {"true", TokenType::KW_TRUE},
        {"false", TokenType::KW_FALSE},
        {"and", TokenType::KW_AND},
        {"or", TokenType::KW_OR},
        {"not", TokenType::KW_NOT},
        {"process", TokenType::KW_PROCESS},
        {"file", TokenType::KW_FILE},
        {"network", TokenType::KW_NETWORK},
        {"registry", TokenType::KW_REGISTRY},
        {"memory", TokenType::KW_MEMORY},
        {"hash", TokenType::KW_HASH},
        {"timestamp", TokenType::KW_TIMESTAMP},
        {"pid", TokenType::KW_PID},
        {"ppid", TokenType::KW_PPID},
        {"name", TokenType::KW_NAME},
        {"path", TokenType::KW_PATH},
        {"cmdline", TokenType::KW_CMDLINE},
        {"user", TokenType::KW_USER},
        {"ip", TokenType::KW_IP},
        {"port", TokenType::KW_PORT},
        {"protocol", TokenType::KW_PROTOCOL},
        {"state", TokenType::KW_STATE},
    };

    auto it = keywords.find(str);
    if (it != keywords.end()) {
        outType = it->second;
        return true;
    }
    return false;
}

Token Lexer::scanIdentifier() {
    size_t start = position_;
    while (std::isalnum(static_cast<unsigned char>(current())) || current() == '_') {
        advance();
    }
    std::string value = input_.substr(start, position_ - start);
    TokenType type;
    if (isKeyword(value, type)) {
        return makeToken(type, value);
    }
    return makeToken(TokenType::IDENTIFIER, value);
}

Token Lexer::scanNumber() {
    size_t start = position_;
    bool hasDot = false;

    while (std::isdigit(static_cast<unsigned char>(current())) || current() == '.') {
        if (current() == '.') {
            if (hasDot) break;
            hasDot = true;
        }
        advance();
    }

    std::string value = input_.substr(start, position_ - start);
    return makeToken(hasDot ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL, value);
}

Token Lexer::scanString() {
    advance(); // Skip opening quote
    size_t start = position_;

    while (current() != '"' && current() != '\0') {
        if (current() == '\\' && peek() != '\0') {
            advance(); // Skip escape character
        }
        advance();
    }

    std::string value = input_.substr(start, position_ - start);
    if (current() == '"') advance(); // Skip closing quote

    return makeToken(TokenType::STRING_LITERAL, value);
}

Token Lexer::scanComment() {
    size_t start = position_;
    advance(); // Skip first /
    if (current() == '/') {
        // Single line comment
        while (current() != '\n' && current() != '\0') {
            advance();
        }
    } else if (current() == '*') {
        // Multi-line comment
        advance(); // Skip *
        while (!(current() == '*' && peek() == '/') && current() != '\0') {
            advance();
        }
        if (current() == '*') {
            advance(); // Skip *
            advance(); // Skip /
        }
    }
    return makeToken(TokenType::COMMENT, input_.substr(start, position_ - start));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (current() != '\0') {
        skipWhitespace();

        if (current() == '\0') break;

        size_t tokenLine = line_;
        size_t tokenCol = column_;

        switch (current()) {
            case '+': advance(); tokens.push_back(makeToken(TokenType::OP_PLUS, "+")); break;
            case '-':
                advance();
                if (current() == '>') { advance(); tokens.push_back(makeToken(TokenType::OP_ARROW, "->")); }
                else { tokens.push_back(makeToken(TokenType::OP_MINUS, "-")); }
                break;
            case '*': advance(); tokens.push_back(makeToken(TokenType::OP_MUL, "*")); break;
            case '/':
                if (peek() == '/' || peek() == '*') {
                    tokens.push_back(scanComment());
                } else {
                    advance(); tokens.push_back(makeToken(TokenType::OP_DIV, "/"));
                }
                break;
            case '%': advance(); tokens.push_back(makeToken(TokenType::OP_MOD, "%")); break;
            case '=':
                advance();
                if (current() == '=') { advance(); tokens.push_back(makeToken(TokenType::OP_EQ, "==")); }
                else { tokens.push_back(makeToken(TokenType::OP_ASSIGN, "=")); }
                break;
            case '!':
                advance();
                if (current() == '=') { advance(); tokens.push_back(makeToken(TokenType::OP_NEQ, "!=")); }
                else { /* Handle error: standalone ! */ }
                break;
            case '<':
                advance();
                if (current() == '=') { advance(); tokens.push_back(makeToken(TokenType::OP_LTE, "<=")); }
                else { tokens.push_back(makeToken(TokenType::OP_LT, "<")); }
                break;
            case '>':
                advance();
                if (current() == '=') { advance(); tokens.push_back(makeToken(TokenType::OP_GTE, ">=")); }
                else { tokens.push_back(makeToken(TokenType::OP_GT, ">")); }
                break;
            case '.': advance(); tokens.push_back(makeToken(TokenType::OP_DOT, ".")); break;
            case ',': advance(); tokens.push_back(makeToken(TokenType::OP_COMMA, ",")); break;
            case ';': advance(); tokens.push_back(makeToken(TokenType::OP_SEMICOLON, ";")); break;
            case ':': advance(); tokens.push_back(makeToken(TokenType::OP_COLON, ":")); break;
            case '(': advance(); tokens.push_back(makeToken(TokenType::OP_LPAREN, "(")); break;
            case ')': advance(); tokens.push_back(makeToken(TokenType::OP_RPAREN, ")")); break;
            case '{': advance(); tokens.push_back(makeToken(TokenType::OP_LBRACE, "{")); break;
            case '}': advance(); tokens.push_back(makeToken(TokenType::OP_RBRACE, "}")); break;
            case '[': advance(); tokens.push_back(makeToken(TokenType::OP_LBRACKET, "[")); break;
            case ']': advance(); tokens.push_back(makeToken(TokenType::OP_RBRACKET, "]")); break;
            case '|': advance(); tokens.push_back(makeToken(TokenType::OP_PIPE, "|")); break;
            case '"': tokens.push_back(scanString()); break;
            default:
                if (std::isalpha(static_cast<unsigned char>(current())) || current() == '_') {
                    tokens.push_back(scanIdentifier());
                } else if (std::isdigit(static_cast<unsigned char>(current()))) {
                    tokens.push_back(scanNumber());
                } else {
                    // Unknown character - skip and continue
                    advance();
                }
                break;
        }
    }

    tokens.push_back(makeToken(TokenType::EOF_TOKEN, ""));
    return tokens;
}

} // namespace jocky