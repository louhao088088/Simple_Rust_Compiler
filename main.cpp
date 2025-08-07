#include "src/lexer/lexer.h"
#include "src/pre_processor/pre_processor.h"

int main() {

    Program program = read_program();
    print_program(program.content);

    vector<Token> tokens = lexer_program(program);
    print_lexer_result(tokens);
    return 0;
}