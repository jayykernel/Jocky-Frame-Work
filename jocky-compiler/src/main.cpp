#include <iostream>
#include "jocky/lexer.h"

int main(int argc, char* argv[]) {
    std::cout << "JOCKY Compiler v0.1.0\n";

    if (argc < 2) {
        std::cerr << "Usage: jocky-compiler <input.jocky>\n";
        return 1;
    }

    std::cout << "Compiling: " << argv[1] << "\n";

    // TODO: Implement actual compilation pipeline
    // 1. Lexical analysis
    // 2. Syntax analysis (parsing)
    // 3. Semantic analysis
    // 4. LLVM IR generation
    // 5. Code optimization
    // 6. Object file generation or JIT execution

    return 0;
}