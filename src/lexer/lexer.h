#pragma once

#include <any>
#include <string>
#include <vector>

using std::string;
using std::vector;

enum class TokenType {

    // symbols
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,
    PERCENT,
    AMPERSAND,
    PIPE,
    CARET,
    COLON,
    QUESTION,

    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,

    ARROW,
    FAT_ARROW,
    COLON_COLON,
    PLUS_EQUAL,
    MINUS_EQUAL,
    STAR_EQUAL,
    SLASH_EQUAL,
    PERCENT_EQUAL,
    AMPERSAND_EQUAL,
    PIPE_EQUAL,
    CARET_EQUAL,
    LESS_LESS,
    GREATER_GREATER,
    LESS_LESS_EQUAL,
    GREATER_GREATER_EQUAL,
    AMPERSAND_AMPERSAND,
    PIPE_PIPE,

    // Literals
    IDENTIFIER,
    STRING,
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

    void print() const;
};

vector<Token> lexer_program(string &program);

void print_lexer_result(const vector<Token> &tokens);