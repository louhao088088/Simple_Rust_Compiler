cmake_minimum_required(VERSION 3.22.1)
project(Simple_RUST_Compiler)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-Wall)
add_compile_options(-O2)
add_compile_options(-g)


add_executable(code 

    src/pre_processor/pre_processor.cpp
    src/lexer/lexer.cpp
    src/ast/ast.cpp
    src/parser/parser.cpp
    src/semantic/semantic.cpp
    src/semantic/name_resolution.cpp
    src/semantic/type_check.cpp
    src/semantic/type_resolve.cpp
    src/tool/number.cpp
    src/error/error.cpp
    src/main.cpp
)

#target_compile_options(code PRIVATE -fsanitize=address,leak,undefined)
#target_link_libraries(code PRIVATE -fsanitize=address,leak,undefined)