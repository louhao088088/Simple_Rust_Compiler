// parser.h
#pragma once

#include "../ast/ast.h"
#include "../error/error.h"

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
    explicit Parser(const std::vector<Token> &tokens, ErrorReporter &error_reporter);
    std::shared_ptr<Program> parse();

  private:
    // State
    const std::vector<Token> &tokens_;
    ErrorReporter &error_reporter_;
    size_t current_ = 0;

    // Pratt parser required types
    using PrefixParseFn = std::function<std::shared_ptr<Expr>()>;
    using InfixParseFn = std::function<std::shared_ptr<Expr>(std::shared_ptr<Expr>)>;

    std::map<TokenType, PrefixParseFn> prefix_parsers_;
    std::map<TokenType, InfixParseFn> infix_parsers_;
    std::map<TokenType, Precedence> precedences_;

    // Pratt parser registration functions
    void register_prefix(TokenType type, PrefixParseFn fn);
    void register_infix(TokenType type, Precedence prec, InfixParseFn fn);

    // Utility functions
    bool is_at_end();
    const Token &peek();
    const Token &peekNext();
    const Token &previous();
    const Token &advance();
    bool check(TokenType type);
    bool match(const std::vector<TokenType> &types);
    bool is_comparison_op(TokenType type) const;
    const Token &consume(TokenType type, const std::string &error_message);
    void report_error(const Token &token, const std::string &message);
    void synchronize();

    // Grammar parsing functions

    // Top level
    std::shared_ptr<Item> parse_item();
    std::shared_ptr<FnDecl> parse_fn_declaration();
    std::shared_ptr<StructDecl> parse_struct_declaration();
    std::shared_ptr<ConstDecl> parse_const_declaration();
    std::shared_ptr<Expr> parse_struct_initializer(std::shared_ptr<Expr> name);
    std::shared_ptr<EnumDecl> parse_enum_declaration();
    std::shared_ptr<EnumVariant> parse_enum_variant();
    std::shared_ptr<ModDecl> parse_mod_declaration();
    std::shared_ptr<TraitDecl> parse_trait_declaration();
    std::shared_ptr<ImplBlock> parse_impl_block();

    // Patterns
    std::shared_ptr<Pattern> parse_pattern();
    std::shared_ptr<Pattern> parse_struct_pattern_body(std::shared_ptr<Expr> path);

    // Statements
    std::shared_ptr<Stmt> parse_statement();
    std::shared_ptr<LetStmt> parse_let_statement();
    std::shared_ptr<ReturnStmt> parse_return_statement();
    std::shared_ptr<BlockStmt> parse_block_statement();
    std::shared_ptr<ExprStmt> parse_expression_statement();
    std::shared_ptr<BreakStmt> parse_break_statement();
    std::shared_ptr<ContinueStmt> parse_continue_statement();

    // Expressions (Pratt Parser)
    std::shared_ptr<Expr> parse_expression(Precedence precedence,
                                           bool allow_struct_literal = false);
    std::shared_ptr<IfExpr> parse_if_expression();
    std::shared_ptr<LoopExpr> parse_loop_expression();
    std::shared_ptr<WhileExpr> parse_while_expression();
    std::shared_ptr<MatchArm> parse_match_arm();
    std::shared_ptr<MatchExpr> parse_match_expression();
    Precedence get_precedence(TokenType type);

    // Type parsing
    std::shared_ptr<TypeNode> parse_type();
};