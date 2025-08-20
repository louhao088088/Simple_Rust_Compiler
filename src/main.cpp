#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pre_processor/pre_processor.h"
#include "semantic/semantic.h"

#include <iostream>

int main() {

    Prog program = read_program();
    std::cout << "--- Source Code ---" << std::endl;
    print_program(program.content);
    std::cout << "\n";

    ErrorReporter lexer_error_reporter;
    vector<Token> tokens = lexer_program(program, lexer_error_reporter);
    std::cout << "--- Lexer Result ---" << std::endl;
    if (lexer_error_reporter.has_errors()) {
        std::cout << "Lexer completed with errors." << std::endl;
        exit(0);
    }
    print_lexer_result(tokens);
    std::cout << "\n";

    std::cout << "--- Parser Result (AST) ---" << std::endl;
    ErrorReporter parser_error_reporter;
    Parser parser(tokens, parser_error_reporter);

    std::shared_ptr<Program> ast = parser.parse();

    if (ast && !parser_error_reporter.has_errors()) {
        ast->print(std::cout);
        std::cout << "\n";
        return 0;
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
        if (parser_error_reporter.has_errors()) {
            std::cout << "Parsing failed with errors." << std::endl;
        } else {
            std::cerr << "Parsing produced a null AST without errors." << std::endl;
        }
    }

    return 0;
}