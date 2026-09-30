#include "jocky/parser.h"
#include <stdexcept>

namespace jocky {

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!isAtEnd()) {
        if (match(TokenType::KW_QUERY)) {
            auto query = parseQuery();
            if (query) {
                program->queries.push_back(std::move(query));
            }
        } else if (match(TokenType::KW_LET)) {
            auto letDecl = parseLetDeclaration();
            if (letDecl) {
                program->globals.push_back(std::move(letDecl));
            }
        } else {
            // Skip unknown tokens
            advance();
        }
    }

    return program;
}

const Token& Parser::current() const {
    return tokens_[pos_];
}

void Parser::advance() {
    if (!isAtEnd()) pos_++;
}

bool Parser::isAtEnd() const {
    return pos_ >= tokens_.size() || current().type == TokenType::EOF_TOKEN;
}

void Parser::expect(TokenType type) {
    if (match(type)) return;
    throw std::runtime_error("Expected token type " + std::to_string(static_cast<int>(type)) +
                            " but got " + std::to_string(static_cast<int>(current().type)));
}

bool Parser::match(TokenType type) {
    if (isAtEnd()) return false;
    if (current().type != type) return false;
    pos_++;
    return true;
}

std::unique_ptr<Program> Parser::parseProgram() {
    return std::make_unique<Program>();
}

std::unique_ptr<QueryDecl> Parser::parseQuery() {
    // query <name> (<param1>, <param2>, ...) {
    //   statements...
    // }

    Location loc = current().loc;

    // Expect query keyword (already consumed)
    if (!match(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected query name after 'query' keyword");
    }
    std::string name = previous().value;

    expect(TokenType::LPAREN);

    std::vector<std::string> parameters;
    if (!match(TokenType::RPAREN)) {
        do {
            if (!match(TokenType::IDENTIFIER)) {
                throw std::runtime_error("Expected parameter name in query parameter list");
            }
            parameters.push_back(previous().value);
        } while (match(TokenType::COMMA));

        expect(TokenType::RPAREN);
    }

    expect(TokenType::LBRACE);

    std::vector<StmtPtr> body;
    while (!match(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    return std::make_unique<QueryDecl>(name, std::move(parameters), std::move(body), loc);
}

std::unique_ptr<LetDecl> Parser::parseLetDeclaration() {
    // let <name> = <expression>;
    // let mut <name> = <expression>;

    Location loc = current().loc;
    bool mutable_ = false;

    // Check for 'mut' keyword
    if (match(TokenType::IDENTIFIER) && current().value == "mut") {
        mutable_ = true;
        advance(); // consume 'mut'
    }

    // Expect identifier
    if (!match(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected identifier in let declaration");
    }
    std::string name = previous().value;

    expect(TokenType::OP_ASSIGN);

    auto initializer = parseExpression();
    if (!initializer) {
        throw std::runtime_error("Expected initializer in let declaration");
    }

    expect(TokenType::SEMICOLON);

    return std::make_unique<LetDecl>(name, std::move(initializer), mutable_, loc);
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::KW_LET)) {
        // Check if it's actually a let declaration or just a misplaced let
        size_t savedPos = pos_;
        if (peekNextIsIdentifierOrMut()) {
            pos_ = savedPos; // Reset position
            return parseLetDeclaration();
        }
        pos_ = savedPos;
    }

    if (match(TokenType::KW_IF)) {
        return parseIfStatement = std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
        return stmt;
    }

    if (match(TokenType::KW_WHILE)) {
        return parseWhileStatement();
    }

    if (match(TokenType::KW_FOR)) {
        return parseForStatement();
    }

    if (match(TokenType::KW_RETURN)) {
        return parseReturnStatement();
    }

    // Expression statement
    auto expr = parseExpression();
    if (expr) {
        expect(TokenType::SEMICOLON);
        // For now, we'll just return null and handle expression statements differently
        // In a real implementation, we'd have an ExpressionStatement node
        // But for simplicity in this phase, we'll ignore expression statements
    }

    return nullptr;
}

bool Parser::peekNextIsIdentifierOrMut() {
    if (pos_ + 1 >= tokens_.size()) return false;

    const Token& next = tokens_[pos_ + 1];
    if (next.type == TokenType::IDENTIFIER) {
        // Could be a variable name or 'mut'
        std::string value = next.value;
        return (value == "mut" ||
               std::isalpha(static_cast<unsigned char>(value[0])) ||
               value[0] == '_');
    }
    return false;
}

std::unique_ptr<IfStmt> Parser::parseIfStatement() {
    // if <condition> {
    //   then branch
    // } else {
    //   else branch
    // }

    Location loc = current().loc;

    expect(TokenType::LPAREN);
    auto condition = parseExpression();
    expect(TokenType::RPAREN);

    expect(TokenType::LBRACE);
    std::vector<StmtPtr> thenBranch;
    while (!match(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            thenBranch.push_back(std::move(stmt));
        }
    }

    std::vector<StmtPtr> elseBranch;
    if (match(TokenType::KW_ELSE)) {
        expect(TokenType::LBRACE);
        while (!match(TokenType::RBRACE) && !isAtEnd()) {
            auto stmt = parseStatement();
            if (stmt) {
                elseBranch.push_back(std::move(stmt));
            }
        }
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

std::unique_ptr<WhileStmt> Parser::parseWhileStatement() {
    // while <condition> {
    //   body
    // }

    Location loc = current().loc;

    expect(TokenType::LPAREN);
    auto condition = parseExpression();
    expect(TokenType::RPAREN);

    expect(TokenType::LBRACE);
    std::vector<StmtPtr> body;
    while (!match(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

std::unique_ptr<ForStmt> Parser::parseForStatement() {
    // for <variable> in <iterable> {
    //   body
    // }

    Location loc = current().loc;

    expect(TokenType::LPAREN);
    if (!match(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected variable name in for loop");
    }
    std::string variable = previous().value;

    expect(TokenType::KW_IN);

    auto iterable = parseExpression();
    expect(TokenType::RPAREN);

    expect(TokenType::LBRACE);
    std::vector<StmtPtr> body;
    while (!match(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    return std::make_unique<ForStmt>(variable, std::move(iterable), std::move(body), loc);
}

std::unique_ptr<ReturnStmt> Parser::parseReturnStatement() {
    // return <expression>;

    Location loc = current().loc;

    auto value = parseExpression();
    expect(TokenType::SEMICOLON);

    return std::make_unique<ReturnStmt>(std::move(value), loc);
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return parseEquality();
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto expr = parseComparison();

    while (match(TokenType::OP_EQ) || match(TokenType::OP_NEQ)) {
        TokenType op = previous().type;
        auto right = parseComparison();

        if (op == TokenType::OP_EQ) {
            expr = std::make_unique<BinaryExpr>(
                BinaryExpr::Op::Eq, std::move(expr), std::move(right), opLocation);
        } else {
            expr = std::make_unique<BinaryExpr>(
                BinaryExpr::Op::Neq, std::move(expr), std::move(right), opLocation);
        }
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseComparison() {
    auto expr = parseTerm();

    while (match(TokenType::OP_LT) || match(TokenType::OP_GT) ||
           match(TokenType::OP_LTE) || match(TokenType::OP_GTE)) {
        TokenType op = previous().type;
        auto right = parseTerm();

        BinaryExpr::Op binaryOp;
        switch (op) {
            case TokenType::OP_LT: binaryOp = BinaryExpr::Op::Lt; break;
            case TokenType::OP_GT: binaryOp = BinaryExpr::Op::Gt; break;
            case TokenType::OP_LTE: binaryOp = BinaryExpr::Op::Lte; break;
            case TokenType::OP_GTE: binaryOp = BinaryExpr::Op::Gte; break;
            default: binaryOp = BinaryExpr::Op::Lt; // Should not happen
        }

        expr = std::make_unique<BinaryExpr>(
            binaryOp, std::move(expr), std::move(right), opLocation);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseTerm() {
    auto expr = parseFactor();

    while (match(TokenType::OP_PLUS) || match(TokenType::OP_MINUS)) {
        TokenType op = previous().type;
        auto right = parseFactor();

        BinaryExpr::Op binaryOp;
        switch (op) {
            case TokenType::OP_PLUS: binaryOp = BinaryExpr::Op::Add; break;
            case TokenType::OP_MINUS: binaryOp = BinaryExpr::Op::Sub; break;
            default: binaryOp = BinaryExpr::Op::Add; // Should not happen
        }

        expr = std::make_unique<BinaryExpr>(
            binaryOp, std::move(expr), std::move(right), opLocation);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseFactor() {
    auto expr = parseUnary();

    while (match(TokenType::OP_MUL) || match(TokenType::OP_DIV) || match(TokenType::OP_MOD)) {
        TokenType op = previous().type;
        auto right = parseUnary();

        BinaryExpr::Op binaryOp;
        switch (op) {
            case TokenType::OP_MUL: binaryOp = BinaryExpr::Op::Mul; break;
            case TokenType::OP_DIV: binaryOp = BinaryExpr::Op::Div; break;
            case TokenType::OP_MOD: binaryOp = BinaryExpr::Op::Mod; break;
            default: binaryOp = BinaryExpr::Op::Mul; // Should not happen
        }

        expr = std::make_unique<BinaryExpr>(
            binaryOp, std::move(expr), std::move(right), opLocation);
    }

    return expr;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match(TokenType::OP_MINUS)) {
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(
            UnaryExpr::Op::Negate, std::move(operand), previous().loc);
    }

    // Handle logical NOT (we'll use '!' token for this)
    if (match(TokenType::OP_NOT)) {
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>(
            UnaryExpr::Op::Not, std::move(operand), previous().loc);
    }

    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        return std::make_unique<LiteralExpr>(
            LiteralExpr::Kind::Integer, previous().value, previous().loc);
    }

    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_unique<LiteralExpr>(
            LiteralExpr::Kind::Float, previous().value, previous().loc);
    }

    if (match(TokenType::STRING_LITERAL)) {
        return std::make_unique<LiteralExpr>(
            LiteralExpr::Kind::String, previous().value, previous().loc);
    }

    if (match(TokenType::KW_TRUE)) {
        return std::make_unique<LiteralExpr>(
            LiteralExpr::Kind::Boolean, "true", previous().loc);
    }

    if (match(TokenType::KW_FALSE)) {
        return std::make_unique<LiteralExpr>(
            LiteralExpr::Kind::Boolean, "false", previous().loc);
    }

    if (match(TokenType::IDENTIFIER)) {
        return parseIdentifier();
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN);
        return expr;
    }

    return nullptr;
}

std::unique_ptr<Expression> Parser::parseIdentifier() {
    Location loc = previous().loc;
    std::string name = previous().value;

    // Check for function call
    if (match(TokenType::LPAREN)) {
        return parseCallExpression(name, loc);
    }

    // Check for member access
    if (match(TokenType::OP_DOT)) {
        return parseMemberAccess(name, loc);
    }

    // Check for index access
    if (match(TokenType::OP_LBRACKET)) {
        return parseIndexExpression(name, loc);
    }

    return std::make_unique<IdentifierExpr>(name, loc);
}

std::unique_ptr<Expression> Parser::parseCallExpression(const std::string& callee, const Location& loc) {
    std::vector<ExprPtr> arguments;

    if (!match(TokenType::RPAREN)) {
        do {
            auto arg = parseExpression();
            if (arg) {
                arguments.push_back(std::move(arg));
            }
        } while (match(TokenType::COMMA));

        expect(TokenType::RPAREN);
    }

    return std::make_unique<CallExpr>(callee, std::move(arguments), loc);
}

std::unique_ptr<Expression> Parser::parseMemberAccess(const std::string& objectName, const Location& loc) {
    // We have the object name as an identifier, now we need the member
    if (!match(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected identifier after '.' in member access");
    }
    std::string member = previous().value;

    // Create identifier expression for the object
    auto objectExpr = std::make_unique<IdentifierExpr>(objectName, loc);

    return std::make_unique<MemberAccessExpr>(std::move(objectExpr), member, loc);
}

std::unique_ptr<Expression> Parser::parseIndexExpression(const std::string& objectName, const Location& loc) {
    auto index = parseExpression();
    expect(TokenType::OP_RBRACKET);

    // Create identifier expression for the object
    auto objectExpr = std::make_unique<IdentifierExpr>(objectName, loc);

    return std::make_unique<IndexExpr>(std::move(objectExpr), std::move(index), loc);
}

// Helper to get the previous token
const Token& Parser::previous() const {
    return tokens_[pos_ - 1];
}

// Helper to get the location of the previous token
const Location& Parser::opLocation() const {
    return previous().loc;
}

} // namespace jocky