#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pre_processor/pre_processor.h"

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

        std::shared_ptr<Program> ast = parser.parse();
        if (ast) {
            ast->print(std::cout);
            std::cout << "\n";

            // Test semantic analysis
            std::cout << "--- Semantic Analysis ---" << std::endl;
            ErrorReporter error_reporter;

            // Name resolution pass
            NameResolutionVisitor name_resolver(error_reporter);
            ast->accept(&name_resolver);

            if (error_reporter.has_errors()) {
                std::cout << "Name resolution completed with errors." << std::endl;
            } else {
                std::cout << "Name resolution completed successfully." << std::endl;

                // Type checking pass
                TypeCheckVisitor type_checker(error_reporter);
                ast->accept(&type_checker);

                if (error_reporter.has_errors()) {
                    std::cout << "Type checking completed with errors." << std::endl;
                } else {
                    std::cout << "Type checking completed successfully." << std::endl;
                }
            }
        } else {
            std::cerr << "Parsing produced a null AST without throwing an error." << std::endl;
        }

    } catch (const std::runtime_error &e) {
        std::cerr << "Parsing Failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}