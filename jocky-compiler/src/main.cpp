#include <iostream>
#include <fstream>
#include <sstream>
#include "jocky/lexer.h"
#include "jocky/parser.h"
#include "jocky/codegen.h"

int main(int argc, char* argv[]) {
    std::cout << "JOCKY Compiler v0.1.0\n";

    if (argc < 2) {
        std::cerr << "Usage: jocky-compiler <input.jocky>\n";
        return 1;
    }

    // Read input file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();
    file.close();

    std::cout << "Compiling: " << argv[1] << "\n";
    std::cout << "Input size: " << input.size() << " bytes\n";

    try {
        // Lexical analysis
        jocky::Lexer lexer(input);
        auto tokens = lexer.tokenize();

        std::cout << "Lexer produced " << tokens.size() << " tokens\n";

        // Parse tokens into AST
        jocky::Parser parser(tokens);
        auto program = parser.parse();

        if (!program) {
            std::cerr << "Error: Failed to parse program\n";
            return 1;
        }

        std::cout << "Parsing successful\n";

        // Generate LLVM IR
        jocky::CodeGen codegen;
        llvm::Module* module = codegen.run(program);

        std::cout << "Code generation successful\n";

        // Print LLVM IR
        module->print(llvm::outs(), nullptr);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}