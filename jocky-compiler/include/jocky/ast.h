#ifndef JOCKY_AST_H
#define JOCKY_AST_H

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace jocky {

// Forward declarations
struct Expression;
struct Statement;
struct QueryDecl;
struct LetDecl;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ReturnStmt;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct IdentifierExpr;
struct LiteralExpr;
struct MemberAccessExpr;
struct IndexExpr;

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;
using QueryDeclPtr = std::shared_ptr<QueryDecl>;
using LetDeclPtr = std::shared_ptr<LetDecl>;
using IfStmtPtr = std::shared_ptr<IfStmt>;
using WhileStmtPtr = std::shared_ptr<WhileStmt>;
using ForStmtPtr = std::shared_ptr<ForStmt>;
using ReturnStmtPtr = std::shared_ptr<ReturnStmt>;
using BinaryExprPtr = std::shared_ptr<BinaryExpr>;
using UnaryExprPtr = std::shared_ptr<UnaryExpr>;
using CallExprPtr = std::shared_ptr<CallExpr>;
using IdentifierExprPtr = std::shared_ptr<IdentifierExpr>;
using LiteralExprPtr = std::shared_ptr<LiteralExpr>;
using MemberAccessExprPtr = std::shared_ptr<MemberAccessExpr>;
using IndexExprPtr = std::shared_ptr<IndexExpr>;

struct Location {
    size_t line;
    size_t column;
    Location(size_t l = 0, size_t c = 0) : line(l), column(c) {}
};

// Base classes
struct Expression {
    Location loc;
    virtual ~Expression() = default;
};

struct Statement {
    Location loc;
    virtual ~Statement() = default;
};

// Literal expressions
struct LiteralExpr : Expression {
    enum class Kind { Integer, Float, String, Boolean };
    Kind kind;
    std::string value;

    LiteralExpr(Kind k, const std::string& v, const Location& loc)
        : Expression(loc), kind(k), value(v) {}
};

// Identifier expression
struct IdentifierExpr : Expression {
    std::string name;
    IdentifierExpr(const std::string& n, const Location& loc)
        : Expression(loc), name(n) {}
};

// Binary expression (e.g., a + b, x == y)
struct BinaryExpr : Expression {
    enum class Op { Add, Sub, Mul, Div, Mod, Eq, Neq, Lt, Gt, Lte, Gte, And, Or };
    Op op;
    ExprPtr left;
    ExprPtr right;

    BinaryExpr(Op o, ExprPtr l, ExprPtr r, const Location& loc)
        : Expression(loc), op(o), left(std::move(l)), right(std::move(r)) {}
};

// Unary expression (e.g., -x, !flag)
struct UnaryExpr : Expression {
    enum class Op { Negate, Not };
    Op op;
    ExprPtr operand;

    UnaryExpr(Op o, ExprPtr e, const Location& loc)
        : Expression(loc), op(o), operand(std::move(e)) {}
};

// Function call expression
struct CallExpr : Expression {
    std::string callee;
    std::vector<ExprPtr> arguments;

    CallExpr(const std::string& c, std::vector<ExprPtr> args, const Location& loc)
        : Expression(loc), callee(c), arguments(std::move(args)) {}
};

// Member access (e.g., process.pid)
struct MemberAccessExpr : Expression {
    ExprPtr object;
    std::string member;

    MemberAccessExpr(ExprPtr obj, const std::string& m, const Location& loc)
        : Expression(loc), object(std::move(obj)), member(m) {}
};

// Index expression (e.g., arr[0])
struct IndexExpr : Expression {
    ExprPtr object;
    ExprPtr index;

    IndexExpr(ExprPtr obj, ExprPtr idx, const Location& loc)
        : Expression(loc), object(std::move(obj)), index(std::move(idx)) {}
};

// Variable declaration
struct LetDecl : Statement {
    std::string name;
    ExprPtr initializer;
    bool mutable_;

    LetDecl(const std::string& n, ExprPtr init, bool mut, const Location& loc)
        : Statement(loc), name(n), initializer(std::move(init)), mutable_(mut) {}
};

// If statement
struct IfStmt : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<StmtPtr> elseBranch;

    IfStmt(ExprPtr cond, std::vector<StmtPtr> thenB, std::vector<StmtPtr> elseB, const Location& loc)
        : Statement(loc), condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
};

// While loop
struct WhileStmt : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;

    WhileStmt(ExprPtr cond, std::vector<StmtPtr> b, const Location& loc)
        : Statement(loc), condition(std::move(cond)), body(std::move(b)) {}
};

// For loop
struct ForStmt : Statement {
    std::string variable;
    ExprPtr iterable;
    std::vector<StmtPtr> body;

    ForStmt(const std::string& var, ExprPtr iter, std::vector<StmtPtr> b, const Location& loc)
        : Statement(loc), variable(var), iterable(std::move(iter)), body(std::move(b)) {}
};

// Return statement
struct ReturnStmt : Statement {
    ExprPtr value;

    ReturnStmt(ExprPtr v, const Location& loc)
        : Statement(loc), value(std::move(v)) {}
};

// Query declaration (the main entry point for JOCKY scripts)
struct QueryDecl : Statement {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StmtPtr> body;

    QueryDecl(const std::string& n, std::vector<std::string> params, std::vector<StmtPtr> b, const Location& loc)
        : Statement(loc), name(n), parameters(std::move(params)), body(std::move(b)) {}
};

// Program (root of AST)
struct Program {
    std::vector<QueryDeclPtr> queries;
    std::vector<LetDeclPtr> globals;
};

} // namespace jocky

#endif // JOCKY_AST_H