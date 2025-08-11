#include "src/pre_processor/pre_processor.h"
#include "src/lexer/lexer.h"
#include "src/parser/parser.h" 
#include <iostream>            

int main() {
  
    Prog program = read_program();
    std::cout << "--- Source Code ---" << std::endl;
    print_program(program.content);
    std::cout << "\n";

    vector<Token> tokens = lexer_program(program);
    std::cout << "--- Lexer Result ---" << std::endl;
    print_lexer_result(tokens);
    std::cout << "\n";

    std::cout << "--- Parser Result (AST) ---" << std::endl;
    try {
        Parser parser(tokens);

        std::unique_ptr<Program> ast = parser.parse();
        if (ast) {
            ast->print(std::cout);
        } else {
            std::cerr << "Parsing produced a null AST without throwing an error." << std::endl;
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Parsing Failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}