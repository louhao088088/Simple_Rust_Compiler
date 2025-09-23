#pragma once

#include "../error/error.h"
#include "../pre_processor/pre_processor.h"

#include <any>
#include <string>
#include <vector>

using std::string;
using std::vector;

enum class TokenType {

    // symbols
    LEFT_PAREN,            //(
    RIGHT_PAREN,           //)
    LEFT_BRACKET,          //[
    RIGHT_BRACKET,         //]
    LEFT_BRACE,            //{
    RIGHT_BRACE,           //}
    COMMA,                 //,
    DOT,                   //.
    MINUS,                 //-
    PLUS,                  //+
    SEMICOLON,             //;
    SLASH,                 // /
    STAR,                  //*
    PERCENT,               //%
    AMPERSAND,             //&
    PIPE,                  //|
    CARET,                 //^
    COLON,                 //:
    QUESTION,              //?
    BANG,                  //!
    BANG_EQUAL,            //!=
    EQUAL,                 //=
    EQUAL_EQUAL,           //==
    GREATER,               //>
    GREATER_EQUAL,         //>=
    LESS,                  //<
    LESS_EQUAL,            //<=
    ARROW,                 //->
    FAT_ARROW,             //=>
    COLON_COLON,           //::
    PLUS_EQUAL,            //+=
    MINUS_EQUAL,           //-=
    STAR_EQUAL,            //*=
    SLASH_EQUAL,           // /=
    PERCENT_EQUAL,         //%=
    AMPERSAND_EQUAL,       //&=
    PIPE_EQUAL,            //|=
    CARET_EQUAL,           //^=
    LESS_LESS,             //<<
    GREATER_GREATER,       //>>
    LESS_LESS_EQUAL,       //<<=
    GREATER_GREATER_EQUAL, //>>=
    AMPERSAND_AMPERSAND,   //&&
    PIPE_PIPE,             //||
    DOT_DOT,               //..
    DOT_DOT_EQUAL,         // ..=

    // Literals
    IDENTIFIER,
    STRING,
    CSTRING,
    RSTRING,
    RCSTRING,
    CHAR,
    NUMBER,

    // Keywords
    AS,
    BREAK,
    CONST,
    CONTINUE,
    CRATE,
    ELSE,
    ENUM,
    EXTERN,
    FALSE,
    FN,
    FOR,
    IF,
    IMPL,
    IN,
    LET,
    LOOP,
    MATCH,
    MOD,
    MOVE,
    MUT,
    PUB,
    REF,
    RETURN,
    SELF,
    SELF_TYPE,
    STATIC,
    STRUCT,
    SUPER,
    TRAIT,
    TRUE,
    TYPE,
    UNSAFE,
    USE,
    WHERE,
    WHILE,
    ASYNC,
    AWAIT,
    DYN,
    ABSTRACT,
    BECOME,
    BOX,
    DO,
    FINAL,
    MACRO,
    OVERRIDE,
    PRIV,
    TYPEOF,
    UNSIZED,
    VIRTUAL,
    YIELD,
    TRY,
    GEN,

    // Misc
    END_OF_FILE,
    UNKNOWN // For error handling
};

string tokenTypeToString(TokenType type);

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    Token() : type(TokenType::UNKNOWN), lexeme(), line(0), column(0) {} // 新增
    Token(TokenType type, std::string lexeme, int line = 0, int column = 0)
        : type(type), lexeme(std::move(lexeme)), line(line), column(column) {}
    void print() const;
};

vector<Token> lexer_program(const Prog &program, ErrorReporter &error_reporter);

void print_lexer_result(const vector<Token> &tokens);