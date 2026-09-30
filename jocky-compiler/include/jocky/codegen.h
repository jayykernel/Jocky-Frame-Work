#ifndef JOCKY_CODEGEN_H
#define JOCKY_CODEGEN_H

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <memory>
#include <unordered_map>
#include <string>
#include "ast.h"

namespace jocky {

class CodeGen {
public:
    CodeGen();
    llvm::Module* run(const std::shared_ptr<Program>& program);

private:
    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;

    // Symbol tables
    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::unordered_map<std::string, std::pair<llvm::Type*, bool>> variableTypes; // type, isMutable

    // Function prototypes for forensic primitives
    void declareForensicFunctions();

    // Code generation methods
    llvm::Value* codegenExpr(const std::shared_ptr<Expression>& expr);
    llvm::Value* codegenStmt(const std::shared_ptr<Statement>& stmt);
    void codegenQuery(const std::shared_ptr<QueryDecl>& query);
    void codegenLetDecl(const std::shared_ptr<LetDecl>& decl);
    void codegenIfStmt(const std::shared_ptr<IfStmt>& stmt);
    void codegenWhileStmt(const std::shared_ptr<WhileStmt>& stmt);
    void codegenForStmt(const std::shared_ptr<ForStmt>& stmt);
    void codegenReturnStmt(const std::shared_ptr<ReturnStmt>& stmt);

    llvm::Value* codegenBinaryExpr(const std::shared_ptr<BinaryExpr>& expr);
    llvm::Value* codegenUnaryExpr(const std::shared_ptr<UnaryExpr>& expr);
    llvm::Value* codegenCallExpr(const std::shared_ptr<CallExpr>& expr);
    llvm::Value* codegenIdentifierExpr(const std::shared_ptr<IdentifierExpr>& expr);
    llvm::Value* codegenLiteralExpr(const std::shared_ptr<LiteralExpr>& expr);
    llvm::Value* codegenMemberAccessExpr(const std::shared_ptr<MemberAccessExpr>& expr);
    llvm::Value* codegenIndexExpr(const std::shared_ptr<IndexExpr>& expr);

    // Type helpers
    llvm::Type* getTypeForLiteral(const LiteralExpr::Kind& kind);
    llvm::Value* getVariableValue(const std::string& name);
    void setVariableValue(const std::string& name, llvm::Value* value, bool isMutable);
};

} // namespace jocky

#endif // JOCKY_CODEGEN_H