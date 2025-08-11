#include "lexer.h"

#include <iostream>
#include <unordered_map>

using std::string;
using std::vector;

void Token::print() const {
    std::cout << "Token:" << tokenTypeToString(type) << ", \"" << lexeme << "\""
              << " at line " << line << ", column " << column << std::endl;
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
    // symbols
    case TokenType::LEFT_PAREN:
        return "LEFT_PAREN";
    case TokenType::RIGHT_PAREN:
        return "RIGHT_PAREN";
    case TokenType::LEFT_BRACE:
        return "LEFT_BRACE";
    case TokenType::RIGHT_BRACE:
        return "RIGHT_BRACE";
    case TokenType::LEFT_BRACKET:
        return "LEFT_BRACKET";
    case TokenType::RIGHT_BRACKET:
        return "RIGHT_BRACKET";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::DOT:
        return "DOT";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::SEMICOLON:
        return "SEMICOLON";
    case TokenType::SLASH:
        return "SLASH";
    case TokenType::STAR:
        return "STAR";
    case TokenType::PERCENT:
        return "PERCENT";
    case TokenType::AMPERSAND:
        return "AMPERSAND";
    case TokenType::PIPE:
        return "PIPE";
    case TokenType::CARET:
        return "CARET";
    case TokenType::COLON:
        return "COLON";
    case TokenType::QUESTION:
        return "QUESTION";
    case TokenType::BANG:
        return "BANG";
    case TokenType::BANG_EQUAL:
        return "BANG_EQUAL";
    case TokenType::EQUAL:
        return "EQUAL";
    case TokenType::EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case TokenType::GREATER:
        return "GREATER";
    case TokenType::GREATER_EQUAL:
        return "GREATER_EQUAL";
    case TokenType::LESS:
        return "LESS";
    case TokenType::LESS_EQUAL:
        return "LESS_EQUAL";
    case TokenType::ARROW:
        return "ARROW";
    case TokenType::FAT_ARROW:
        return "FAT_ARROW";
    case TokenType::COLON_COLON:
        return "COLON_COLON";
    case TokenType::PLUS_EQUAL:
        return "PLUS_EQUAL";
    case TokenType::MINUS_EQUAL:
        return "MINUS_EQUAL";
    case TokenType::STAR_EQUAL:
        return "STAR_EQUAL";
    case TokenType::SLASH_EQUAL:
        return "SLASH_EQUAL";
    case TokenType::PERCENT_EQUAL:
        return "PERCENT_EQUAL";
    case TokenType::AMPERSAND_EQUAL:
        return "AMPERSAND_EQUAL";
    case TokenType::PIPE_EQUAL:
        return "PIPE_EQUAL";
    case TokenType::CARET_EQUAL:
        return "CARET_EQUAL";
    case TokenType::LESS_LESS:
        return "LESS_LESS";
    case TokenType::GREATER_GREATER:
        return "GREATER_GREATER";
    case TokenType::LESS_LESS_EQUAL:
        return "LESS_LESS_EQUAL";
    case TokenType::GREATER_GREATER_EQUAL:
        return "GREATER_GREATER_EQUAL";
    case TokenType::AMPERSAND_AMPERSAND:
        return "AMPERSAND_AMPERSAND";
    case TokenType::PIPE_PIPE:
        return "PIPE_PIPE";

    // Literals
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::STRING:
        return "STRING";
    case TokenType::CHAR:
        return "CHAR";
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::BYTE:
        return "BYTE";
    case TokenType::CSTRING:
        return "CSTRING";
    case TokenType::BSTRING:
        return "BSTRING";

    // Keywords
    case TokenType::AS:
        return "AS";
    case TokenType::BREAK:
        return "BREAK";
    case TokenType::CONST:
        return "CONST";
    case TokenType::CONTINUE:
        return "CONTINUE";
    case TokenType::CRATE:
        return "CRATE";
    case TokenType::ELSE:
        return "ELSE";
    case TokenType::ENUM:
        return "ENUM";
    case TokenType::EXTERN:
        return "EXTERN";
    case TokenType::FALSE:
        return "FALSE";
    case TokenType::FN:
        return "FN";
    case TokenType::FOR:
        return "FOR";
    case TokenType::IF:
        return "IF";
    case TokenType::IMPL:
        return "IMPL";
    case TokenType::IN:
        return "IN";
    case TokenType::LET:
        return "LET";
    case TokenType::LOOP:
        return "LOOP";
    case TokenType::MATCH:
        return "MATCH";
    case TokenType::MOD:
        return "MOD";
    case TokenType::MOVE:
        return "MOVE";
    case TokenType::MUT:
        return "MUT";
    case TokenType::PUB:
        return "PUB";
    case TokenType::REF:
        return "REF";
    case TokenType::RETURN:
        return "RETURN";
    case TokenType::SELF:
        return "SELF";
    case TokenType::SELF_TYPE:
        return "SELF_TYPE";
    case TokenType::STATIC:
        return "STATIC";
    case TokenType::STRUCT:
        return "STRUCT";
    case TokenType::SUPER:
        return "SUPER";
    case TokenType::TRAIT:
        return "TRAIT";
    case TokenType::TRUE:
        return "TRUE";
    case TokenType::TYPE:
        return "TYPE";
    case TokenType::UNSAFE:
        return "UNSAFE";
    case TokenType::USE:
        return "USE";
    case TokenType::WHERE:
        return "WHERE";
    case TokenType::WHILE:
        return "WHILE";
    case TokenType::ASYNC:
        return "ASYNC";
    case TokenType::AWAIT:
        return "AWAIT";
    case TokenType::DYN:
        return "DYN";
    case TokenType::ABSTRACT:
        return "ABSTRACT";
    case TokenType::BECOME:
        return "BECOME";
    case TokenType::BOX:
        return "BOX";
    case TokenType::DO:
        return "DO";
    case TokenType::FINAL:
        return "FINAL";
    case TokenType::MACRO:
        return "MACRO";
    case TokenType::OVERRIDE:
        return "OVERRIDE";
    case TokenType::PRIV:
        return "PRIV";
    case TokenType::TYPEOF:
        return "TYPEOF";
    case TokenType::UNSIZED:
        return "UNSIZED";
    case TokenType::VIRTUAL:
        return "VIRTUAL";
    case TokenType::YIELD:
        return "YIELD";
    case TokenType::TRY:
        return "TRY";
    case TokenType::GEN:
        return "GEN";
    case TokenType::END_OF_FILE:
        return "END_OF_FILE";

    default:
        return "UNKNOWN";
    }
}

static std::unordered_map<std::string, TokenType> keywords = {

    {"as", TokenType::AS},
    {"break", TokenType::BREAK},
    {"const", TokenType::CONST},
    {"continue", TokenType::CONTINUE},
    {"crate", TokenType::CRATE},
    {"else", TokenType::ELSE},
    {"enum", TokenType::ENUM},
    {"extern", TokenType::EXTERN},
    {"false", TokenType::FALSE},
    {"fn", TokenType::FN},
    {"for", TokenType::FOR},
    {"if", TokenType::IF},
    {"impl", TokenType::IMPL},
    {"in", TokenType::IN},
    {"let", TokenType::LET},
    {"loop", TokenType::LOOP},
    {"match", TokenType::MATCH},
    {"mod", TokenType::MOD},
    {"move", TokenType::MOVE},
    {"mut", TokenType::MUT},
    {"pub", TokenType::PUB},
    {"ref", TokenType::REF},
    {"return", TokenType::RETURN},
    {"self", TokenType::SELF},
    {"Self", TokenType::SELF_TYPE},
    {"static", TokenType::STATIC},
    {"struct", TokenType::STRUCT},
    {"super", TokenType::SUPER},
    {"trait", TokenType::TRAIT},
    {"true", TokenType::TRUE},
    {"type", TokenType::TYPE},
    {"unsafe", TokenType::UNSAFE},
    {"use", TokenType::USE},
    {"where", TokenType::WHERE},
    {"while", TokenType::WHILE},
    {"async", TokenType::ASYNC},
    {"await", TokenType::AWAIT},
    {"dyn", TokenType::DYN},
    {"abstract", TokenType::ABSTRACT},
    {"become", TokenType::BECOME},
    {"box", TokenType::BOX},
    {"do", TokenType::DO},
    {"final", TokenType::FINAL},
    {"macro", TokenType::MACRO},
    {"override", TokenType::OVERRIDE},
    {"priv", TokenType::PRIV},
    {"typeof", TokenType::TYPEOF},
    {"unsized", TokenType::UNSIZED},
    {"virtual", TokenType::VIRTUAL},
    {"yield", TokenType::YIELD},
    {"try", TokenType::TRY},
    {"gen", TokenType::GEN}

};

static std::unordered_map<std::string, TokenType> symbols = {

    {">>=", TokenType::GREATER_GREATER_EQUAL},
    {"<<=", TokenType::LESS_LESS_EQUAL},
    {"!=", TokenType::BANG_EQUAL},
    {"==", TokenType::EQUAL_EQUAL},
    {">=", TokenType::GREATER_EQUAL},
    {"<=", TokenType::LESS_EQUAL},
    {"->", TokenType::ARROW},
    {"=>", TokenType::FAT_ARROW},
    {"::", TokenType::COLON_COLON},
    {"+=", TokenType::PLUS_EQUAL},
    {"-=", TokenType::MINUS_EQUAL},
    {"*=", TokenType::STAR_EQUAL},
    {"/=", TokenType::SLASH_EQUAL},
    {"%=", TokenType::PERCENT_EQUAL},
    {"&=", TokenType::AMPERSAND_EQUAL},
    {"|=", TokenType::PIPE_EQUAL},
    {"^=", TokenType::CARET_EQUAL},
    {"<<", TokenType::LESS_LESS},
    {">>", TokenType::GREATER_GREATER},
    {"&&", TokenType::AMPERSAND_AMPERSAND},
    {"||", TokenType::PIPE_PIPE},
    {"(", TokenType::LEFT_PAREN},
    {")", TokenType::RIGHT_PAREN},
    {"{", TokenType::LEFT_BRACE},
    {"}", TokenType::RIGHT_BRACE},
    {"[", TokenType::LEFT_BRACKET},
    {"]", TokenType::RIGHT_BRACKET},
    {",", TokenType::COMMA},
    {".", TokenType::DOT},
    {"-", TokenType::MINUS},
    {"+", TokenType::PLUS},
    {";", TokenType::SEMICOLON},
    {"/", TokenType::SLASH},
    {"*", TokenType::STAR},
    {"%", TokenType::PERCENT},
    {"&", TokenType::AMPERSAND},
    {"|", TokenType::PIPE},
    {"^", TokenType::CARET},
    {":", TokenType::COLON},
    {"?", TokenType::QUESTION},
    {"!", TokenType::BANG},
    {"=", TokenType::EQUAL},
    {">", TokenType::GREATER},
    {"<", TokenType::LESS}

};

vector<Token> lexer_program(const Prog &program) {
    bool in_string = 0;
    bool in_string2 = 0;
    bool trans = 0;
    size_t i = 0;
    string token = "";
    Token new_token;
    vector<Token> result;

    while (i < program.content.size()) {
        char ch = program.content[i];
        char next_ch = (i + 1 < program.content.size()) ? program.content[i + 1] : '\0';

        string tmp = "";
        tmp += ch;
        if (in_string) {
            token += ch;
            if (trans) {
                trans = 0;
            } else if (ch == '\\') {
                trans = 1;

            } else if (ch == '"') {
                in_string = 0;
                if (token[0] == 'b')
                    new_token.type = TokenType::BSTRING;
                else if (token[0] == 'c')
                    new_token.type = TokenType::CSTRING;
                else
                    new_token.type = TokenType::STRING;
                new_token.lexeme = token;
                new_token.line = program.positions[i].first;
                new_token.column = program.positions[i].second;
                result.push_back(new_token);
                token = "";
            }
            i++;
        } else if (in_string2) {
            token += ch;
            if (trans) {
                trans = 0;
            } else if (ch == '\\') {
                trans = 1;

            } else if (ch == '\'') {
                in_string2 = 0;
                if (token[0] == 'b')
                    new_token.type = TokenType::BYTE;
                else
                    new_token.type = TokenType::CHAR;
                new_token.lexeme = token;
                new_token.line = program.positions[i].first;
                new_token.column = program.positions[i].second;
                result.push_back(new_token);

                token = "";
            }
            i++;
        } else if (ch == '"' && (token.size() == 0 ||
                                 (token.size() == 1 && (token[0] == 'b' || token[0] == 'c')))) {
            token += ch;
            in_string = 1;
            i++;
        } else if (ch == '\'' && (token.size() == 0 || (token.size() == 1 && token[0] == 'b'))) {
            token += ch;
            in_string2 = 1;
            i++;
        } else if (symbols.find(tmp) != symbols.end() && token.length() == 0) {
            switch (ch) {
            // Single-character symbols
            case '(':
                new_token.type = TokenType::LEFT_PAREN;
                new_token.lexeme = '(';
                break;
            case ')':
                new_token.type = TokenType::RIGHT_PAREN;
                new_token.lexeme = ')';
                break;
            case '{':
                new_token.type = TokenType::LEFT_BRACE;
                new_token.lexeme = '{';
                break;
            case '}':
                new_token.type = TokenType::RIGHT_BRACE;
                new_token.lexeme = '}';
                break;
            case '[':
                new_token.type = TokenType::LEFT_BRACKET;
                new_token.lexeme = '[';
                break;
            case ']':
                new_token.type = TokenType::RIGHT_BRACKET;
                new_token.lexeme = ']';
                break;
            case ',':
                new_token.type = TokenType::COMMA;
                new_token.lexeme = ',';
                break;
            case ';':
                new_token.type = TokenType::SEMICOLON;
                new_token.lexeme = ';';
                break;
            case '?':
                new_token.type = TokenType::QUESTION;
                new_token.lexeme = '?';
                break;
            case '.':
                new_token.type = TokenType::DOT;
                new_token.lexeme = '.';
                break;

            // Symbols with potential multi-character tokens
            case '!':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::BANG_EQUAL;
                    new_token.lexeme = "!=";
                } else {
                    new_token.type = TokenType::BANG;
                    new_token.lexeme = "!";
                }
                break;
            case '=':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::EQUAL_EQUAL;
                    new_token.lexeme = "==";
                } else if (next_ch == '>') {
                    i++;
                    new_token.type = TokenType::ARROW;
                    new_token.lexeme = "->";
                } else {
                    new_token.type = TokenType::EQUAL;
                    new_token.lexeme = "=";
                }
                break;
            case '<':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::LESS_EQUAL;
                    new_token.lexeme = "<=";
                } else if (next_ch == '<') {
                    i++;
                    new_token.type = TokenType::LESS_LESS;
                    new_token.lexeme = "<<";
                    if (i + 2 < program.content.size() && program.content[i + 2] == '=') {
                        i++;
                        new_token.type = TokenType::LESS_LESS_EQUAL;
                        new_token.lexeme = "<<=";
                    }

                } else {
                    new_token.type = TokenType::LESS;
                    new_token.lexeme = "<";
                }
                break;
            case '>':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::GREATER_EQUAL;
                    new_token.lexeme = ">=";
                } else if (next_ch == '>') {
                    i++;
                    new_token.type = TokenType::GREATER_GREATER;
                    new_token.lexeme = ">>";
                    if (i + 2 < program.content.size() && program.content[i + 2] == '=') {
                        i++;
                        new_token.type = TokenType::GREATER_GREATER_EQUAL;
                        new_token.lexeme = ">>=";
                    }
                } else {
                    new_token.type = TokenType::GREATER;
                    new_token.lexeme = ">";
                }
                break;
            case '&':
                if (next_ch == '&') {
                    i++;
                    new_token.type = TokenType::AMPERSAND_AMPERSAND;
                    new_token.lexeme = "&&";
                } else if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::AMPERSAND_EQUAL;
                    new_token.lexeme = "&=";
                } else {
                    new_token.type = TokenType::AMPERSAND;
                    new_token.lexeme = "&";
                }
                break;
            case '|':
                if (next_ch == '|') {
                    i++;
                    new_token.type = TokenType::PIPE_PIPE;
                    new_token.lexeme = "||";
                } else if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::PIPE_EQUAL;
                    new_token.lexeme = "|=";
                } else {
                    new_token.type = TokenType::PIPE;
                    new_token.lexeme = "|";
                }
                break;
            case '+':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::PLUS_EQUAL;
                    new_token.lexeme = "+=";
                } else {
                    new_token.type = TokenType::PLUS;
                    new_token.lexeme = "+";
                }
                break;
            case '-':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::MINUS_EQUAL;
                    new_token.lexeme = "-=";
                } else if (next_ch == '>') {
                    i++;
                    new_token.type = TokenType::ARROW;
                    new_token.lexeme = "->";
                } else {
                    new_token.type = TokenType::MINUS;
                    new_token.lexeme = "-";
                }
                break;
            case '*':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::STAR_EQUAL;
                    new_token.lexeme = "*=";
                } else {
                    new_token.type = TokenType::STAR;
                    new_token.lexeme = "*";
                }
                break;
            case '/':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::SLASH_EQUAL;
                    new_token.lexeme = "/=";
                } else {
                    new_token.type = TokenType::SLASH;
                    new_token.lexeme = "/";
                }
                break;
            case '%':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::PERCENT_EQUAL;
                    new_token.lexeme = "%=";
                } else {
                    new_token.type = TokenType::PERCENT;
                    new_token.lexeme = "%";
                }
                break;
            case '^':
                if (next_ch == '=') {
                    i++;
                    new_token.type = TokenType::CARET_EQUAL;
                    new_token.lexeme = "^=";
                } else {
                    new_token.type = TokenType::CARET;
                    new_token.lexeme = "^";
                }
                break;
            case ':':
                if (next_ch == ':') {
                    i++;
                    new_token.type = TokenType::COLON_COLON;
                    new_token.lexeme = "::";
                } else {
                    new_token.type = TokenType::COLON;
                    new_token.lexeme = ":";
                }
                break;
            }
            new_token.line = program.positions[i].first;
            new_token.column = program.positions[i].second;
            i++;
            result.push_back(new_token);

        } else if (!(ch >= '0' && ch <= '9') && !(ch >= 'A' && ch <= 'Z') &&
                   !(ch >= 'a' && ch <= 'z') && ch != '_' && token.length() > 0) {
            if (token[0] >= '0' && token[0] <= '9') {
                new_token.type = TokenType::NUMBER;
            } else {
                new_token.type = TokenType::IDENTIFIER;
                auto it = keywords.find(token);
                if (it != keywords.end()) {
                    new_token.type = it->second;
                }
            }
            new_token.lexeme = token;
            new_token.line = program.positions[i - 1].first;
            new_token.column = program.positions[i - 1].second;
            result.push_back(new_token);

            token = "";
        } else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') ||
                   (ch >= 'a' && ch <= 'z') || ch == '_') {
            token += ch, i++;
        } else
            i++;
    }
    if (token.length() > 0) {
        if (token[0] >= '0' && token[0] <= '9') {
            new_token.type = TokenType::NUMBER;
        } else {
            new_token.type = TokenType::IDENTIFIER;
            auto it = keywords.find(token);
            if (it != keywords.end()) {
                new_token.type = it->second;
            }
        }
        new_token.lexeme = token;
        new_token.line = program.positions[i - 1].first;
        new_token.column = program.positions[i - 1].second;
        result.push_back(new_token);
    }
    return result;
}

void print_lexer_result(const vector<Token> &tokens) {
    puts("First Step lexer result:");

    for (const auto &token : tokens) {
        token.print();
    }

    puts("");
    puts("");
}