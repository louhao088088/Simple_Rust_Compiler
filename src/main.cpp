#include "lexer/lexer.h"
#include "parser/parser.h"
#include "pre_processor/pre_processor.h"
#include "semantic/semantic.h"

#include <iostream>

int main() {

    Prog program = read_program();
    std::cerr << "--- Source Code ---" << std::endl;
    print_program(program.content);
    std::cerr << "\n";

    ErrorReporter lexer_error_reporter;
    vector<Token> tokens = lexer_program(program, lexer_error_reporter);
    std::cerr << "--- Lexer Result ---" << std::endl;
    if (lexer_error_reporter.has_errors()) {
        std::cerr << "Lexer completed with errors." << std::endl;
        return 1;
    }
    print_lexer_result(tokens);
    std::cerr << "\n";

    std::cerr << "--- Parser Result (AST) ---" << std::endl;
    ErrorReporter parser_error_reporter;
    Parser parser(tokens, parser_error_reporter);

    std::shared_ptr<Program> ast = parser.parse();

    if (ast && !parser_error_reporter.has_errors()) {
        ast->print(std::cerr);
        std::cerr << "\n";
    } else {
        if (parser_error_reporter.has_errors()) {
            std::cerr << "Parsing failed with errors." << std::endl;
            return 1;
        } else {
            std::cerr << "Parsing produced a null AST without errors." << std::endl;
        }
    }

    // Test semantic analysis
    std::cerr << "--- Semantic Analysis ---" << std::endl;
    ErrorReporter error_reporter;
    Semantic(ast, error_reporter);
    if (error_reporter.has_errors()) {
        std::cerr << "Semantic analysis completed with errors." << std::endl;
        return 1;
    } else {
        std::cerr << "Semantic analysis completed successfully." << std::endl;
    }

    return 0;
}