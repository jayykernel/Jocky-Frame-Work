#ifndef JOCKY_LEXER_H
#define JOCKY_LEXER_H

#include <string>
#include <vector>

namespace jocky {

enum class TokenType {
    // End of file
    EOF_TOKEN,

    // Identifiers and literals
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    // Keywords
    KW_QUERY,
    KW_LET,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_IN,
    KW_RETURN,
    KW_TRUE,
    KW_FALSE,
    KW_AND,
    KW_OR,
    KW_NOT,

    // Forensic primitives
    KW_PROCESS,
    KW_FILE,
    KW_NETWORK,
    KW_REGISTRY,
    KW_MEMORY,
    KW_HASH,
    KW_TIMESTAMP,
    KW_PID,
    KW_PPID,
    KW_NAME,
    KW_PATH,
    KW_CMDLINE,
    KW_USER,
    KW_IP,
    KW_PORT,
    KW_PROTOCOL,
    KW_STATE,

    // Operators
    OP_ASSIGN,      // =
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_MUL,         // *
    OP_DIV,         // /
    OP_MOD,         // %
    OP_EQ,          // ==
    OP_NEQ,         // !=
    OP_LT,          // <
    OP_GT,          // >
    OP_LTE,         // <=
    OP_GTE,         // >=
    OP_DOT,         // .
    OP_COMMA,       // ,
    OP_SEMICOLON,   // ;
    OP_COLON,       // :
    OP_LPAREN,      // (
    OP_RPAREN,      // )
    OP_LBRACE,      // {
    OP_RBRACE,      // }
    OP_LBRACKET,    // [
    OP_RBRACKET,    // ]
    OP_ARROW,       // ->
    OP_PIPE,        // |

    // Special
    COMMENT,
    WHITESPACE
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;

    Token(TokenType t, const std::string& v, size_t l, size_t c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& input);
    std::vector<Token> tokenize();

private:
    const std::string& input_;
    size_t position_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    Token makeToken(TokenType type, const std::string& value);
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
    Token scanComment();
    bool isKeyword(const std::string& str, TokenType& outType) const;
};

} // namespace jocky

#endif // JOCKY_LEXER_H