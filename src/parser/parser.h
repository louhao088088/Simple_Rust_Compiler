// parser.h
#pragma once

#include "ast.h"

#include <functional>
#include <map>
#include <vector>

// 运算符优先级
enum Precedence {
    NONE,
    ASSIGNMENT,  // = = += -= *= /= %= &= |= ^= <<= >>=
    RANGE,       // .. ..=
    OR,          // ||
    AND,         // &&
    COMPARISON,  // < > <= >=  == !=
    BITWISE_OR,  // |
    BITWISE_XOR, // ^
    BITWISE_AND, // &
    SHIFT,       // << >>
    TERM,        // + -
    FACTOR,      // * /
    AS,          // as
    UNARY,       // ! - *
    CALL,        // . () []
    PATH         // ::
};

class Parser {
  public:
    explicit Parser(const std::vector<Token> &tokens);
    std::unique_ptr<Program> parse();

  private:
    // --- 状态 ---
    const std::vector<Token> &tokens_;
    size_t current_ = 0;

    // --- Pratt 解析器所需类型 ---
    using PrefixParseFn = std::function<std::unique_ptr<Expr>()>;
    using InfixParseFn = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>)>;

    std::map<TokenType, PrefixParseFn> prefix_parsers_;
    std::map<TokenType, InfixParseFn> infix_parsers_;
    std::map<TokenType, Precedence> precedences_;

    // --- Pratt 解析器注册函数 ---
    void register_prefix(TokenType type, PrefixParseFn fn);
    void register_infix(TokenType type, Precedence prec, InfixParseFn fn);

    // --- 工具函数 ---
    bool is_at_end();
    const Token &peek();
    const Token &previous();
    const Token &advance();
    bool check(TokenType type);
    bool match(const std::vector<TokenType> &types);
    const Token &consume(TokenType type, const std::string &error_message);
    std::runtime_error error(const Token &token, const std::string &message);
    void synchronize();

    // --- 语法规则解析函数 ---

    // 顶层
    std::unique_ptr<Item> parse_item();
    std::unique_ptr<FnDecl> parse_fn_declaration();

    // 语句
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<LetStmt> parse_let_statement();
    std::unique_ptr<ReturnStmt> parse_return_statement();
    std::unique_ptr<BlockStmt> parse_block_statement();
    std::unique_ptr<ExprStmt> parse_expression_statement();
    std::unique_ptr<BreakStmt> parse_break_statement();
    std::unique_ptr<ContinueStmt> parse_continue_statement();

    // 表达式 (Pratt Parser)
    std::unique_ptr<Expr> parse_expression(Precedence precedence);
    Precedence get_precedence(TokenType type);

    // 类型解析
    std::unique_ptr<TypeNode> parse_type();
    std::unique_ptr<TypeNode> parse_primary_type();
};