// parser.cpp
#include "parser.h"

#include "ast.h"

using std::string;
using std::vector;

// Constructor: set up all parsing rules
Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens) {
    // Register Pratt parser rules

    // Register prefix parsing functions
    register_prefix(TokenType::IDENTIFIER,
                    [this] { return std::make_unique<VariableExpr>(previous()); });
    register_prefix(TokenType::NUMBER,
                    [this] { return std::make_unique<LiteralExpr>(previous()); });
    register_prefix(TokenType::STRING,
                    [this] { return std::make_unique<LiteralExpr>(previous()); });
    register_prefix(TokenType::TRUE, [this] { return std::make_unique<LiteralExpr>(previous()); });
    register_prefix(TokenType::FALSE, [this] { return std::make_unique<LiteralExpr>(previous()); });

    register_prefix(TokenType::MINUS, [this] {
        auto op = previous();
        auto right = parse_expression(Precedence::UNARY);
        return std::make_unique<UnaryExpr>(op, std::move(right));
    });
    register_prefix(TokenType::BANG, [this] {
        auto op = previous();
        auto right = parse_expression(Precedence::UNARY);
        return std::make_unique<UnaryExpr>(op, std::move(right));
    });

    register_prefix(TokenType::IF, [this] {
        puts("why there If");
        return parse_if_expression();
    });
    register_prefix(TokenType::WHILE, [this] { return parse_while_expression(); });
    register_prefix(TokenType::LOOP, [this] { return parse_loop_expression(); });

    register_prefix(TokenType::LEFT_PAREN, [this] -> std::unique_ptr<Expr> {
        if (check(TokenType::RIGHT_PAREN)) {
            consume(TokenType::RIGHT_PAREN, "Unclosed unit literal.");
            return std::make_unique<UnitExpr>();
        }
        auto expr = parse_expression(Precedence::NONE);
        if (match({TokenType::COMMA})) {
            std::vector<std::unique_ptr<Expr>> elements;
            elements.push_back(std::move(expr));

            while (!check(TokenType::RIGHT_PAREN) && !is_at_end()) {
                elements.push_back(parse_expression(Precedence::NONE));
                if (!check(TokenType::RIGHT_PAREN)) {
                    consume(TokenType::COMMA, "Expect ',' between tuple elements.");
                }
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple.");
            return std::make_unique<TupleExpr>(std::move(elements));
        } else {

            consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return std::make_unique<GroupingExpr>(std::move(expr));
        }
    });

    register_prefix(TokenType::LEFT_BRACKET, [this] -> std::unique_ptr<Expr> {
        if (check(TokenType::RIGHT_BRACKET)) {
            Token closing = consume(TokenType::RIGHT_BRACKET, "Unclosed empty array literal.");
            return std::make_unique<ArrayLiteralExpr>(std::vector<std::unique_ptr<Expr>>{});
        }
        auto first_expr = parse_expression(Precedence::NONE);
        if (match({TokenType::SEMICOLON})) {
            auto count_expr = parse_expression(Precedence::NONE);
            consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array initializer expression.");
            return std::make_unique<ArrayInitializerExpr>(std::move(first_expr),
                                                          std::move(count_expr));
        } else {
            std::vector<std::unique_ptr<Expr>> elements;
            elements.push_back(std::move(first_expr));
            while (match({TokenType::COMMA})) {
                if (check(TokenType::RIGHT_BRACKET)) {
                    break;
                }
                elements.push_back(parse_expression(Precedence::NONE));
            }
            Token closing = consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array literal.");
            return std::make_unique<ArrayLiteralExpr>(std::move(elements));
        }
    });

    register_infix(TokenType::AS, Precedence::AS, [this](auto left) -> std::unique_ptr<Expr> {
        auto target_type = parse_type();
        if (!target_type) {
            throw error(peek(), "Expect a type after 'as' keyword.");
        }
        return std::make_unique<AsExpr>(std::move(left), std::move(target_type));
    });

    // Register infix parsing functions and precedence
    register_infix(TokenType::PLUS, Precedence::TERM, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::TERM);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::MINUS, Precedence::TERM, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::TERM);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::STAR, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::SLASH, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PERCENT, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND, Precedence::BITWISE_AND, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_AND);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE, Precedence::BITWISE_OR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_OR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::CARET, Precedence::BITWISE_XOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_XOR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_LESS, Precedence::SHIFT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::SHIFT);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::GREATER_GREATER, Precedence::SHIFT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::SHIFT);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::EQUAL_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::BANG_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND_AMPERSAND, Precedence::AND, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::AND);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE_PIPE, Precedence::OR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::OR);
        return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::LEFT_PAREN, Precedence::CALL, [this](auto callee) {
        std::vector<std::unique_ptr<Expr>> arguments;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                arguments.push_back(parse_expression(Precedence::NONE));
            } while (match({TokenType::COMMA}));
        }
        Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
        return std::make_unique<CallExpr>(std::move(callee), std::move(arguments));
    });

    register_infix(TokenType::DOT, Precedence::CALL, [this](auto left) {
        Token field_name = consume(TokenType::IDENTIFIER, "Expect field name after '.'.");
        return std::make_unique<FieldAccessExpr>(std::move(left), field_name);
    });

    register_infix(TokenType::LEFT_BRACKET, Precedence::CALL, [this](auto left) {
        auto index_expr = parse_expression(Precedence::NONE);

        Token closing_bracket = consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
        return std::make_unique<IndexExpr>(std::move(left), std::move(index_expr));
    });
    register_infix(TokenType::EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto value = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<AssignmentExpr>(std::move(left), std::move(value));
    });
    register_infix(TokenType::PLUS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::MINUS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::STAR_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::SLASH_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PERCENT_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::CARET_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_LESS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER_GREATER_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_unique<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
}

// Implementation of registration functions
void Parser::register_prefix(TokenType type, PrefixParseFn fn) { prefix_parsers_[type] = fn; }
void Parser::register_infix(TokenType type, Precedence prec, InfixParseFn fn) {
    infix_parsers_[type] = fn;
    precedences_[type] = prec;
}
std::unique_ptr<TypeNode> Parser::parse_type() { return parse_primary_type(); }

std::unique_ptr<TypeNode> Parser::parse_primary_type() {
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<TypeNameNode>(previous());
    }
    if (match({TokenType::BANG})) {
        return std::make_unique<TypeNameNode>(previous());
    }
    if (match({TokenType::LEFT_BRACKET})) {
        auto element_type = parse_type();
        consume(TokenType::SEMICOLON, "Expect ';' in array type definition.");
        auto size_expr = parse_expression(Precedence::NONE);
        consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array type.");
        return std::make_unique<ArrayTypeNode>(std::move(element_type), std::move(size_expr));
    }

    if (match({TokenType::LEFT_PAREN})) {
        if (match({TokenType::RIGHT_PAREN})) {
            return std::make_unique<UnitTypeNode>();
        } else if (check(TokenType::IDENTIFIER)) {
            std::vector<std::unique_ptr<TypeNode>> elements;
            do {
                elements.push_back(parse_primary_type());
            } while (match({TokenType::COMMA}));
            consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple type.");
            return std::make_unique<TupleTypeNode>(std::move(elements));
        }
    }

    throw error(peek(), "Expected a type.");
}

// Main parsing loop
std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    while (!is_at_end()) {
        try {
            program->items.push_back(parse_item());
        } catch (const std::runtime_error &e) {
            std::cerr << e.what() << std::endl;
            synchronize();
        }
    }
    return program;
}

// Recursive descent implementation
std::unique_ptr<Item> Parser::parse_item() {

    if (peek().type == TokenType::FN) {
        return parse_fn_declaration();
    } else if (peek().type == TokenType::STRUCT) {

        return parse_struct_declaration();
    }
    throw error(peek(), "Expect a top-level item like 'fn'.");
}

std::unique_ptr<FnDecl> Parser::parse_fn_declaration() {
    consume(TokenType::FN, "Expect 'fn'.");
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");

    std::vector<Token> params;
    std::vector<std::unique_ptr<TypeNode>> param_types;
    std::optional<std::unique_ptr<TypeNode>> return_type;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expect parameter name."));
            consume(TokenType::COLON, "Expect ': ' after parameter name.");
            param_types.push_back(parse_type());
        } while (match({TokenType::COMMA}));
    }

    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    if (check(TokenType::ARROW)) {
        advance();
        return_type = parse_type();
    }
    auto body = parse_block_statement();
    return std::make_unique<FnDecl>(name, std::move(params), std::move(param_types),
                                    std::move(return_type), std::move(body));
}

std::unique_ptr<StructDecl> Parser::parse_struct_declaration() {
    consume(TokenType::STRUCT, "Expect 'struct' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect struct name.");

    if (peek().type == TokenType::LEFT_BRACE) {
        consume(TokenType::LEFT_BRACE, "Expect '{' before struct body.");
        std::vector<Field> fields;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
            consume(TokenType::COLON, "Expect ':' after field name.");
            auto field_type = parse_type();
            fields.push_back({field_name, std::move(field_type)});
            if (!match({TokenType::COMMA}))
                break;
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' after struct body.");
        return std::make_unique<StructDecl>(name, std::move(fields));

    } else if (peek().type == TokenType::LEFT_PAREN) {
        consume(TokenType::LEFT_PAREN, "Expect '(' for tuple struct.");
        std::vector<std::unique_ptr<TypeNode>> tuple_fields;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                tuple_fields.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after tuple struct fields.");
        consume(TokenType::SEMICOLON, "Expect ';' after tuple struct declaration.");
        return std::make_unique<StructDecl>(name, std::move(tuple_fields));

    } else {
        consume(TokenType::SEMICOLON, "Expect ';' for unit-like struct declaration.");
        return std::make_unique<StructDecl>(name);
    }
}

std::unique_ptr<Expr> Parser::parse_struct_initializer(std::unique_ptr<Expr> name) {
    consume(TokenType::LEFT_BRACE, "Expect '{' for struct initializer.");
    std::vector<FieldInitializer> fields;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        Token field_name = advance();
        if (field_name.type != TokenType::IDENTIFIER && field_name.type != TokenType::NUMBER) {
            throw error(field_name, "Expect field name or index in struct initializer.");
        }

        consume(TokenType::COLON, "Expect ':' after field name.");
        auto value = parse_expression(Precedence::NONE);

        fields.push_back({field_name, std::move(value)});
        if (!check(TokenType::RIGHT_BRACE)) {
            consume(TokenType::COMMA, "Expect ',' after field value.");
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to close struct initializer.");

    return std::make_unique<StructInitializerExpr>(std::move(name), std::move(fields));
}

std::unique_ptr<BlockStmt> Parser::parse_block_statement() {
    consume(TokenType::LEFT_BRACE, "Expect '{' to start a block.");
    auto block = std::make_unique<BlockStmt>();
    puts("hello block");
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        block->statements.push_back(parse_statement());
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to end a block.");
    if (!block->statements.empty()) {
        if (auto *last_stmt = dynamic_cast<ExprStmt *>(block->statements.back().get())) {
            if (!last_stmt->has_semicolon) {
                block->final_expr = std::move(last_stmt->expression);
                block->statements.pop_back();
            }
        }
    }

    return block;
}

std::unique_ptr<Stmt> Parser::parse_statement() {
    if (peek().type == TokenType::LET)
        return parse_let_statement();
    if (peek().type == TokenType::RETURN)
        return parse_return_statement();
    if (peek().type == TokenType::STRUCT) {
        return std::make_unique<ItemStmt>(parse_struct_declaration());
    }

    return parse_expression_statement();
}

std::unique_ptr<LetStmt> Parser::parse_let_statement() {
    consume(TokenType::LET, "Expect 'let'.");
    bool is_mutable = match({TokenType::MUT});
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");

    std::optional<std::unique_ptr<TypeNode>> type_annotation;
    if (match({TokenType::COLON})) {
        type_annotation = parse_type();
    }

    std::optional<std::unique_ptr<Expr>> initializer;
    if (match({TokenType::EQUAL})) {
        initializer = parse_expression(Precedence::NONE);
    }
    consume(TokenType::SEMICOLON, "Expect ';' after let statement.");
    return std::make_unique<LetStmt>(name, is_mutable, std::move(type_annotation),
                                     std::move(initializer));
}

std::unique_ptr<ReturnStmt> Parser::parse_return_statement() {
    Token keyword = consume(TokenType::RETURN, "Expect 'return'.");
    std::optional<std::unique_ptr<Expr>> value;
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression(Precedence::NONE);
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

std::unique_ptr<BreakStmt> Parser::parse_break_statement() {
    Token keyword = consume(TokenType::BREAK, "Expect 'break'.");
    std::optional<std::unique_ptr<Expr>> value;
    if (!check(TokenType::SEMICOLON) && !is_at_end()) {
        value = parse_expression(Precedence::NONE);
    }
    consume(TokenType::SEMICOLON, "Expect ';' after break statement.");
    return std::make_unique<BreakStmt>(std::move(value));
}

std::unique_ptr<ContinueStmt> Parser::parse_continue_statement() {
    Token keyword = consume(TokenType::CONTINUE, "Expect 'continue'.");
    consume(TokenType::SEMICOLON, "Expect ';' after continue statement.");
    return std::make_unique<ContinueStmt>();
}

std::unique_ptr<ExprStmt> Parser::parse_expression_statement() {
    auto expr = parse_expression(Precedence::NONE);
    if (match({TokenType::SEMICOLON})) {
        return std::make_unique<ExprStmt>(std::move(expr), true);
    } else {
        return std::make_unique<ExprStmt>(std::move(expr), false);
    }
    return std::make_unique<ExprStmt>(std::move(expr), true);
}

// Core of Pratt parser
Precedence Parser::get_precedence(TokenType type) {
    if (precedences_.count(type)) {
        return precedences_[type];
    }
    return Precedence::NONE;
}

std::unique_ptr<Expr> Parser::parse_expression(Precedence precedence) {
    advance();
    TokenType prefix_type = previous().type;
    if (prefix_parsers_.find(prefix_type) == prefix_parsers_.end()) {
        throw error(previous(), "Expect an expression.");
    }

    auto left = prefix_parsers_[prefix_type]();

    if (peek().type == TokenType::LEFT_BRACE) {
        bool is_valid_struct_path = dynamic_cast<VariableExpr *>(left.get()) != nullptr;

        if (is_valid_struct_path) {
            if (Precedence::CALL > precedence) {
                
                left = parse_struct_initializer(std::move(left));
            }
        }
    }

    while (precedence < get_precedence(peek().type)) {
        advance();
        TokenType infix_type = previous().type;
        if (infix_parsers_.find(infix_type) == infix_parsers_.end()) {
            return left;
        }
        left = infix_parsers_[infix_type](std::move(left));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parse_if_expression() {
    auto condition = parse_expression(Precedence::NONE);
    puts("I am in IF");
    auto then_branch = parse_block_statement();
    puts("I am out if");
    std::optional<std::unique_ptr<Stmt>> else_branch;
    if (match({TokenType::ELSE})) {
        if (peek().type == TokenType::IF) {
            else_branch = parse_statement();
        } else {
            else_branch = parse_block_statement();
        }
    }
    return std::make_unique<IfExpr>(std::move(condition), std::move(then_branch),
                                    std::move(else_branch));
}

std::unique_ptr<Expr> Parser::parse_loop_expression() {
    auto body = parse_block_statement();
    return std::make_unique<LoopExpr>(std::move(body));
}

std::unique_ptr<Expr> Parser::parse_while_expression() {
    auto condition = parse_expression(Precedence::NONE);
    auto body = parse_block_statement();
    return std::make_unique<WhileExpr>(std::move(condition), std::move(body));
}

// tools
bool Parser::is_at_end() {
    return current_ >= tokens_.size() || peek().type == TokenType::END_OF_FILE;
}
const Token &Parser::peek() { return tokens_[current_]; }
const Token &Parser::previous() { return tokens_[current_ - 1]; }
const Token &Parser::advance() {
    if (!is_at_end())
        current_++;
    return previous();
}
bool Parser::check(TokenType type) {
    if (is_at_end())
        return false;
    return peek().type == type;
}
bool Parser::is_comparison_op(TokenType type) const {
    switch (type) {
    case TokenType::GREATER:
    case TokenType::GREATER_EQUAL:
    case TokenType::LESS:
    case TokenType::LESS_EQUAL:
    case TokenType::EQUAL_EQUAL:
    case TokenType::BANG_EQUAL:
        return true;
    default:
        return false;
    }
}
bool Parser::match(const std::vector<TokenType> &types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}
const Token &Parser::consume(TokenType type, const std::string &error_message) {
    if (check(type))
        return advance();
    throw error(peek(), error_message);
}
std::runtime_error Parser::error(const Token &token, const std::string &message) {
    return std::runtime_error("[line " + std::to_string(token.line) + "] Error at '" +
                              token.lexeme + "': " + message);
}
void Parser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous().type == TokenType::SEMICOLON)
            return;
        switch (peek().type) {
        case TokenType::FN:
        case TokenType::LET:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::LOOP:
        case TokenType::RETURN:
            return;
        default:
            break;
        }
        advance();
    }
}