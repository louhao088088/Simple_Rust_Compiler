// parser.h
#pragma once

#include "ast.h"

#include <functional>
#include <map>
#include <vector>

// Operator precedence
enum Precedence {
    NONE,
    ASSIGNMENT,  // = += -= *= /= %= &= |= ^= <<= >>=
    RANGE,       // .. ..=
    OR,          // ||
    AND,         // &&
    COMPARISON,  // < > <= >=  == !=
    BITWISE_OR,  // |
    BITWISE_XOR, // ^
    BITWISE_AND, // &
    SHIFT,       // << >>
    TERM,        // + -
    FACTOR,      // * / %
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
    // State
    const std::vector<Token> &tokens_;
    size_t current_ = 0;

    // Pratt parser required types
    using PrefixParseFn = std::function<std::unique_ptr<Expr>()>;
    using InfixParseFn = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>)>;

    std::map<TokenType, PrefixParseFn> prefix_parsers_;
    std::map<TokenType, InfixParseFn> infix_parsers_;
    std::map<TokenType, Precedence> precedences_;

    // Pratt parser registration functions
    void register_prefix(TokenType type, PrefixParseFn fn);
    void register_infix(TokenType type, Precedence prec, InfixParseFn fn);

    // Utility functions
    bool is_at_end();
    const Token &peek();
    const Token &previous();
    const Token &advance();
    bool check(TokenType type);
    bool match(const std::vector<TokenType> &types);
    bool is_comparison_op(TokenType type) const;
    const Token &consume(TokenType type, const std::string &error_message);
    std::runtime_error error(const Token &token, const std::string &message);
    void synchronize();

    // Grammar parsing functions

    // Top level
    std::unique_ptr<Item> parse_item();
    std::unique_ptr<FnDecl> parse_fn_declaration();
    std::unique_ptr<StructDecl> parse_struct_declaration();
    std::unique_ptr<ConstDecl> parse_const_declaration();
    std::unique_ptr<Expr> parse_struct_initializer(std::unique_ptr<Expr> name);
    std::unique_ptr<EnumDecl> parse_enum_declaration();
    std::unique_ptr<EnumVariant> parse_enum_variant();
    std::unique_ptr<ModDecl> parse_mod_declaration();

    // Patterns
    std::unique_ptr<Pattern> parse_pattern();
    std::unique_ptr<Pattern> parse_struct_pattern_body(std::unique_ptr<Expr> path);

    // Statements
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<LetStmt> parse_let_statement();
    std::unique_ptr<ReturnStmt> parse_return_statement();
    std::unique_ptr<BlockStmt> parse_block_statement();
    std::unique_ptr<ExprStmt> parse_expression_statement();
    std::unique_ptr<BreakStmt> parse_break_statement();
    std::unique_ptr<ContinueStmt> parse_continue_statement();

    // Expressions (Pratt Parser)
    std::unique_ptr<Expr> parse_expression(Precedence precedence,
                                           bool allow_struct_literal = false);
    std::unique_ptr<IfExpr> parse_if_expression();
    std::unique_ptr<LoopExpr> parse_loop_expression();
    std::unique_ptr<WhileExpr> parse_while_expression();
    std::unique_ptr<MatchArm> parse_match_arm();
    std::unique_ptr<MatchExpr> parse_match_expression();
    Precedence get_precedence(TokenType type);

    // Type parsing
    std::unique_ptr<TypeNode> parse_type();
};