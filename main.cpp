#include "src/pre_processor/pre_processor.h"
#include "src/lexer/lexer.h"

int main() {

    
    string program = read_program();
    print_program(program);


    vector<Token> tokens = lexer_program(program);
    print_lexer_result(tokens);
    return 0;
}