#include "jocky/codegen.h"
#include <llvm/IR/Verifier.h>
#include <iostream>

namespace jocky {

CodeGen::CodeGen()
    : builder_(context_)
{
    module_ = std::make_unique<llvm::Module>("jocky_module", context_);
    builder_.SetInsertPoint(llvm::BasicBlock::Create(context_, "entry",
                                                    llvm::Function::Create(
                                                        llvm::FunctionType::get(
                                                            llvm::Type::getVoidTy(context_),
                                                            false
                                                        ),
                                                        llvm::Function::ExternalLinkage,
                                                        "main",
                                                        module_.get()
                                                    )
                                                   ));
}

llvm::Module* CodeGen::run(const std::shared_ptr<Program>& program) {
    // Declare forensic functions
    declareForensicFunctions();

    // Generate code for global variables
    for (const auto& global : program->globals) {
        codegenLetDecl(global);
    }

    // Generate code for queries (these become functions)
    for (const auto& query : program->queries) {
        codegenQuery(query);
    }

    // Verify the generated code
    llvm::verifyModule(*module_);

    return module_.get();
}

void CodeGen::declareForensicFunctions() {
    // Declare built-in forensic functions that will be available to JOCKY scripts

    // Process enumeration
    llvm::FunctionType* processEnumType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),  // Returns void (we'll fill array parameter)
        false
    );
    llvm::Function::Create(processEnumType,
                          llvm::Function::ExternalLinkage,
                          "enumerate_processes",
                          module_.get());

    // File operations
    llvm::FunctionType* fileHashType = llvm::FunctionType::get(
        llvm::Type::getInt8Ty(context_),  // Returns hash as i8* (string)
        { llvm::Type::getInt8Ty(context_) },  // Takes file path as i8*
        false
    );
    llvm::Function::Create(fileHashType,
                          llvm::Function::ExternalLinkage,
                          "hash_file",
                          module_.get());

    // Network operations
    llvm::FunctionType* networkEnumType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        false
    );
    llvm::Function::Create(networkEnumType,
                          llvm::Function::ExternalLinkage,
                          "enumerate_network_connections",
                          module_.get());

    // Registry operations (Windows)
    llvm::FunctionType* registryGetType = llvm::FunctionType::get(
        llvm::Type::getInt8Ty(context_),
        { llvm::Type::getInt8Ty(context_), llvm::Type::getInt8Ty(context_) },  // key, value
        false
    );
    llvm::Function::Create(registryGetType,
                          llvm::Function::ExternalLinkage,
                          "registry_get_value",
                          module_.get());
}

llvm::Value* CodeGen::codegenExpr(const std::shared_ptr<Expression>& expr) {
    if (!expr) return nullptr;

    switch (expr->loc.line) {  // Using line as a rough type identifier - not ideal but works for demo
        // Actually, we need to use RTTI or visitor pattern. Let's use dynamic_cast
        default: break;
    }

    // Use dynamic_cast to determine expression type
    if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        return codegenBinaryExpr(binaryExpr);
    }
    if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        return codegenUnaryExpr(unaryExpr);
    }
    if (auto callExpr = std::dynamic_pointer_cast<CallExpr>(expr)) {
        return codegenCallExpr(callExpr);
    }
    if (auto identifierExpr = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
        return codegenIdentifierExpr(identifierExpr);
    }
    if (auto literalExpr = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        return codegenLiteralExpr(literalExpr);
    }
    if (auto memberAccessExpr = std::dynamic_pointer_cast<MemberAccessExpr>(expr)) {
        return codegenMemberAccessExpr(memberAccessExpr);
    }
    if (auto indexExpr = std::dynamic_pointer_cast<IndexExpr>(expr)) {
        return codegenIndexExpr(indexExpr);
    }

    return nullptr;
}

llvm::Value* CodeGen::codegenStmt(const std::shared_ptr<Statement>& stmt) {
    if (!stmt) return nullptr;

    if (auto letDecl = std::dynamic_pointer_cast<LetDecl>(stmt)) {
        return codegenLetDecl(letDecl);
    }
    if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        return codegenIfStmt(ifStmt);
    }
    if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        return codegenWhileStmt(whileStmt);
    }
    if (auto forStmt = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        return codegenForStmt(forStmt);
    }
    if (auto returnStmt = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        return codegenReturnStmt(returnStmt);
    }

    return nullptr;
}

void CodeGen::codegenQuery(const std::shared_ptr<QueryDecl>& query) {
    // Create function type: void function()
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context_),
        false
    );

    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        query->name,
        module_.get()
    );

    // Create basic block for function body
    llvm::BasicBlock* block = llvm::BasicBlock::Create(context_, "entry", function);
    builder_.SetInsertPoint(block);

    // Generate code for function body
    for (const auto& stmt : query->body) {
        codegenStmt(stmt);
    }

    // Add return void at end if not already present
    builder_.CreateRetVoid();
}

void CodeGen::codegenLetDecl(const std::shared_ptr<LetDecl>& decl) {
    llvm::Value* value = codegenExpr(decl->initializer);
    if (!value) return;

    llvm::Type* type = value->getType();
    llvm::AllocaInst* allocation = builder_.CreateAlloca(type, nullptr, decl->name);
    builder_.CreateStore(value, allocation);

    // Store in symbol table
    namedValues[decl->name] = allocation;
    variableTypes[decl->name] = std::make_pair(type, decl->mutable_);
}

void CodeGen::codegenIfStmt(const std::shared_ptr<IfStmt>& stmt) {
    llvm::Value* conditionV = codegenExpr(stmt->condition);
    if (!conditionV) return;

    llvm::Function* function = builder_.GetInsertBlock()->getParent();

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(context_, "then", function);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(context_, "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context_, "ifcont");

    builder_.CreateCondBr(conditionV, thenBB, elseBB);

    // Then branch
    builder_.SetInsertPoint(thenBB);
    for (const auto& stmt : stmt->thenBranch) {
        codegenStmt(stmt);
    }
    builder_.CreateBr(mergeBB);
    thenBB = builder_.GetInsertBlock();

    // Else branch
    function->insert(function->end(), elseBB);
    builder_.SetInsertPoint(elseBB);
    for (const auto& stmt : stmt->elseBranch) {
        codegenStmt(stmt);
    }
    builder_.CreateBr(mergeBB);
    elseBB = builder_.GetInsertBlock();

    // Merge block
    function->insert(function->end(), mergeBB);
    builder_.SetInsertPoint(mergeBB);
}

void CodeGen::codegenWhileStmt(const std::shared_ptr<WhileStmt>& stmt) {
    llvm::Function* function = builder_.GetInsertBlock()->getParent();

    llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(context_, "loop", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context_, "loopbody");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context_, "afterloop");

    builder_.CreateBr(loopBB);
    builder_.SetInsertPoint(loopBB);

    llvm::Value* conditionV = codegenExpr(stmt->condition);
    if (!conditionV) return;

    builder_.CreateCondBr(conditionV, bodyBB, afterBB);

    // Body
    builder_.SetInsertPoint(bodyBB);
    for (const auto& stmt : stmt->body) {
        codegenStmt(stmt);
    }
    builder_.CreateBr(loopBB);

    // After loop
    function->insert(function->end(), afterBB);
    builder_.SetInsertPoint(afterBB);
}

void CodeGen::codegenForStmt(const std::shared_ptr<ForStmt>& stmt) {
    // For now, treat as while loop (simplified)
    // In a full implementation, we'd handle the iteration properly
    llvm::Function* function = builder_.GetInsertBlock()->getParent();

    llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(context_, "loop", function);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(context_, "loopbody");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context_, "afterloop");

    builder_.CreateBr(loopBB);
    builder_.SetInsertPoint(loopBB);

    // Condition (always true for now - we'd need to implement iterable checking)
    llvm::Value* conditionV = llvm::ConstantInt::get(context_, llvm::APInt(1, 1));
    builder_.CreateCondBr(conditionV, bodyBB, afterBB);

    // Body
    builder_.SetInsertPoint(bodyBB);
    for (const auto& stmt : stmt->body) {
        codegenStmt(stmt);
    }
    builder_.CreateBr(loopBB);

    // After loop
    function->insert(function->end(), afterBB);
    builder_.SetInsertPoint(afterBB);
}

void CodeGen::codegenReturnStmt(const std::shared_ptr<ReturnStmt>& stmt) {
    llvm::Value* value = codegenExpr(stmt->value);
    if (value) {
        builder_.CreateRet(value);
    } else {
        builder_.CreateRetVoid();
    }
}

llvm::Value* CodeGen::codegenBinaryExpr(const std::shared_ptr<BinaryExpr>& expr) {
    llvm::Value* leftV = codegenExpr(expr->left);
    llvm::Value* rightV = codegenExpr(expr->right);
    if (!leftV || !rightV) return nullptr;

    switch (expr->op) {
        case BinaryExpr::Op::Add:
            return builder_.CreateAdd(leftV, rightV, "addtmp");
        case BinaryExpr::Op::Sub:
            return builder_.CreateSub(leftV, rightV, "subtmp");
        case BinaryExpr::Op::Mul:
            return builder_.CreateMul(leftV, rightV, "multmp");
        case BinaryExpr::Op::Div:
            return builder_.CreateUDiv(leftV, rightV, "divtmp");  // Assuming unsigned for simplicity
        case BinaryExpr::Op::Mod:
            return builder_.CreateURem(leftV, rightV, "modtmp");
        case BinaryExpr::Op::Eq:
            return builder_.CreateICmpEQ(leftV, rightV, "eqtmp");
        case BinaryExpr::Op::Neq:
            return builder_.CreateICmpNE(leftV, rightV, "neqtmp");
        case BinaryExpr::Op::Lt:
            return builder_.CreateICmpSLT(leftV, rightV, "lttmp");  // Signed comparison
        case BinaryExpr::Op::Gt:
            return builder_.CreateICmpSGT(leftV, rightV, "gttmp");
        case BinaryExpr::Op::Lte:
            return builder_.CreateICmpSLE(leftV, rightV, "letmp");
        case BinaryExpr::Op::Gte:
            return builder_.CreateICmpSGE(leftV, rightV, "getmp");
        case BinaryExpr::Op::And:
            return builder_.CreateAnd(leftV, rightV, "andtmp");
        case BinaryExpr::Op::Or:
            return builder_.CreateOr(leftV, rightV, "ortmp");
        default:
            return nullptr;
    }
}

llvm::Value* CodeGen::codegenUnaryExpr(const std::shared_ptr<UnaryExpr>& expr) {
    llvm::Value* operandV = codegenExpr(expr->operand);
    if (!operandV) return nullptr;

    switch (expr->op) {
        case UnaryExpr::Op::Negate:
            return builder_.CreateNeg(operandV, "negtmp");
        case UnaryExpr::Op::Not:
            return builder_.CreateNot(operandV, "nottmp");
        default:
            return nullptr;
    }
}

llvm::Value* CodeGen::codegenCallExpr(const std::shared_ptr<CallExpr>& expr) {
    // Look up the function in the module
    llvm::Function* function = module_->getFunction(expr->callee);
    if (!function) {
        // Function not found - could be a built-in we need to declare
        // For now, return null
        return nullptr;
    }

    std::vector<llvm::Value*> argsV;
    for (const auto& arg : expr->arguments) {
        llvm::Value* argV = codegenExpr(arg);
        if (!argV) return nullptr;
        argsV.push_back(argV);
    }

    return builder_.CreateCall(function, argsV, "calltmp");
}

llvm::Value* CodeGen::codegenIdentifierExpr(const std::shared_ptr<IdentifierExpr>& expr) {
    llvm::Value* value = getVariableValue(expr->name);
    return value;
}

llvm::Value* CodeGen::codegenLiteralExpr(const std::shared_ptr<LiteralExpr>& expr) {
    switch (expr->kind) {
        case LiteralExpr::Kind::Integer:
            return llvm::ConstantInt::get(context_, llvm::APInt(64, std::stoll(expr->value)));
        case LiteralExpr::Kind::Float:
            return llvm::ConstantFP::get(context_, llvm::APFloat(std::stod(expr->value)));
        case LiteralExpr::Kind::String:
            return builder_.CreateGlobalStringPtr(expr->value, "str");
        case LiteralExpr::Kind::Boolean:
            return llvm::ConstantInt::get(context_, llvm::APInt(1, expr->value == "true" ? 1 : 0));
        default:
            return nullptr;
    }
}

llvm::Value* CodeGen::codegenMemberAccessExpr(const std::shared_ptr<MemberAccessExpr>& expr) {
    // For now, we'll implement a simplified version
    // In reality, this would depend on the type of the object
    llvm::Value* objectV = codegenExpr(expr->object);
    if (!objectV) return nullptr;

    // For simplicity, we'll treat member access as a function call
    // e.g., process.pid -> get_process_pid(process)
    std::string funcName = "get_" + expr->member;
    llvm::Function* function = module_->getFunction(funcName);
    if (!function) {
        // Try to declare it on the fly (simplified)
        // In reality, we'd need proper type information
        return nullptr;
    }

    return builder_.CreateCall(function, objectV, "membercall");
}

llvm::Value* CodeGen::codegenIndexExpr(const std::shared_ptr<IndexExpr>& expr) {
    // Array indexing - simplified implementation
    llvm::Value* objectV = codegenExpr(expr->object);
    llvm::Value* indexV = codegenExpr(expr->index);
    if (!objectV || !indexV) return nullptr;

    // For now, return a placeholder
    // In reality, we'd need to know the element type and do GEP
    return llvm::ConstantInt::get(context_, llvm::APInt(32, 0));
}

llvm::Type* CodeGen::getTypeForLiteral(const LiteralExpr::Kind& kind) {
    switch (kind) {
        case LiteralExpr::Kind::Integer:
            return llvm::Type::getInt64Ty(context_);
        case LiteralExpr::Kind::Float:
            return llvm::Type::getDoubleTy(context_);
        case LiteralExpr::Kind::String:
            return llvm::Type::getInt8PtrTy(context_);
        case LiteralExpr::Kind::Boolean:
            return llvm::Type::getInt1Ty(context_);
        default:
            return llvm::Type::getInt64Ty(context_);
    }
}

llvm::Value* CodeGen::getVariableValue(const std::string& name) {
    auto it = namedValues.find(name);
    if (it == namedValues.end()) {
        return nullptr;
    }
    return builder_.CreateLoad(it->second->getAllocatedType(), it->second, name.c_str());
}

void CodeGen::setVariableValue(const std::string& name, llvm::Value* value, bool isMutable) {
    auto it = namedValues.find(name);
    if (it != namedValues.end()) {
        builder_.CreateStore(value, it->second);
    }
}

} // namespace jocky