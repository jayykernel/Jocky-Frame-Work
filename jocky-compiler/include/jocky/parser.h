#ifndef JOCKY_PARSER_H
#define JOCKY_PARSER_H

#include <vector>
#include <memory>
#include "ast.h"
#include "lexer.h"

namespace jocky {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;

    // Current token
    const Token& current() const;
    // Advance to next token
    void advance();
    // Check if we have more tokens
    bool isAtEnd() const;
    // Expect a specific token type and advance
    void expect(TokenType type);
    // Check if current token matches type (without consuming)
    bool match(TokenType type);
    // Check if current token matches type and consume if true
    bool consume(TokenType type);

    // Parsing functions
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<QueryDecl> parseQuery();
    std::unique_ptr<LetDecl> parseLetDeclaration();
    std::unique_ptr<Statement> parseStatement();
    std::unique_ptr<Expression> parseExpression();
    std::unique_ptr<Expression> parseEquality();
    std::unique_ptr<Expression> parseComparison();
    std::unique_ptr<Expression> parseTerm();
    std::unique_ptr<Expression> parseFactor();
    std::unique_ptr<Expression> parseUnary();
    std::unique_ptr<Expression> parsePrimary();
    std::unique_ptr<Expression> parseLiteral();
    std::unique_ptr<Expression> parseIdentifier();
    std::unique_ptr<Expression> parseCallExpression();
    std::unique_ptr<Expression> parseMemberAccess();
    std::unique_ptr<Expression> parseIndexExpression();

    // Helper for parsing binary expressions
    template<typename Func>
    std::unique_ptr<Expression> parseBinary(Func& func, TokenType op1, TokenType op2);
};

} // namespace jocky

#endif // JOCKY_PARSER_H