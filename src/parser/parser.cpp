// parser.cpp
#include "parser.h"

#include "ast.h"

// --- 构造函数：设置好所有解析规则 ---
Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens) {
    // --- 注册 Pratt 解析器规则 ---

    // 注册前缀解析函数
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

    register_prefix(TokenType::LEFT_PAREN, [this] {
        auto expr = parse_expression(Precedence::NONE);
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
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
    register_prefix(TokenType::IF, [this] {
        auto condition = parse_expression(Precedence::NONE);

        auto then_branch = parse_block_statement();

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
    });

    register_prefix(TokenType::WHILE, [this] {
        auto condition = parse_expression(Precedence::NONE);
        auto body = parse_block_statement();
        return std::make_unique<WhileExpr>(std::move(condition), std::move(body));
    });

    register_prefix(TokenType::LOOP, [this] {
        auto body = parse_block_statement();
        return std::make_unique<LoopExpr>(std::move(body));
    });

    // 注册中缀解析函数和优先级
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
}

// --- 注册函数的实现 ---
void Parser::register_prefix(TokenType type, PrefixParseFn fn) { prefix_parsers_[type] = fn; }
void Parser::register_infix(TokenType type, Precedence prec, InfixParseFn fn) {
    infix_parsers_[type] = fn;
    precedences_[type] = prec;
}
std::unique_ptr<TypeNode> Parser::parse_type() {
    // Rust 的类型系统也可能有优先级，但这里我们先简化
    return parse_primary_type();
}

std::unique_ptr<TypeNode> Parser::parse_primary_type() {
    // 如果是标识符，那么就是命名类型
    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<TypeNameNode>(previous());
    }

    // 如果是左方括号，那么就是数组类型
    if (match({TokenType::LEFT_BRACKET})) {
        auto element_type = parse_type();
        consume(TokenType::SEMICOLON, "Expect ';' in array type definition.");
        auto size_expr = parse_expression(Precedence::NONE);
        consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array type.");
        return std::make_unique<ArrayTypeNode>(std::move(element_type), std::move(size_expr));
    }

    // 如果是左括号，可能是单元类型 () 或元组类型 (T1, T2)
    if (match({TokenType::LEFT_PAREN})) {
        if (match({TokenType::RIGHT_PAREN})) {
            // 这是单元类型 ()
            return std::make_unique<UnitTypeNode>();
        } else if (check(TokenType::IDENTIFIER)) {
            // 这是元组类型 (T1, T2)
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
// --- 主解析循环 ---
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

// --- 递归下降的实现 ---
std::unique_ptr<Item> Parser::parse_item() {
    if (peek().type == TokenType::FN) {
        return parse_fn_declaration();
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

std::unique_ptr<Stmt> Parser::parse_statement() {
    if (peek().type == TokenType::LET)
        return parse_let_statement();
    if (peek().type == TokenType::RETURN)
        return parse_return_statement();
    if (peek().type == TokenType::BREAK)
        return parse_break_statement();
    if (peek().type == TokenType::CONTINUE)
        return parse_continue_statement();
    if (peek().type == TokenType::LEFT_BRACE)
        return parse_block_statement();
    if (peek().type == TokenType::IF || peek().type == TokenType::LOOP ||
        peek().type == TokenType::WHILE) {
        auto expr = parse_expression(Precedence::NONE);
        return std::make_unique<ExprStmt>(std::move(expr));
    }
    return parse_expression_statement();
}

std::unique_ptr<BlockStmt> Parser::parse_block_statement() {
    consume(TokenType::LEFT_BRACE, "Expect '{' to start a block.");
    auto block = std::make_unique<BlockStmt>();
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        if (peek().type >= TokenType::AS && peek().type <= TokenType::GEN) {
            block->statements.push_back(parse_statement());
            continue;
        }

        auto expr = parse_expression(Precedence::NONE);
        if (match({TokenType::SEMICOLON})) {
            block->statements.push_back(std::make_unique<ExprStmt>(std::move(expr)));
        } else {
            block->final_expr = std::move(expr);
            break;
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to end a block.");
    return block;
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
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// --- Pratt 解析器的核心 ---
Precedence Parser::get_precedence(TokenType type) {
    if (precedences_.count(type)) {
        return precedences_[type];
    }
    return Precedence::NONE;
}

std::unique_ptr<Expr> Parser::parse_expression(Precedence precedence) {
    advance();
    TokenType prefix_type = previous().type;
    if (!prefix_parsers_.count(prefix_type)) {
        throw error(previous(), "Expect an expression.");
    }
    auto left = prefix_parsers_[prefix_type]();

    while (precedence < get_precedence(peek().type)) {
        advance();
        TokenType infix_type = previous().type;
        left = infix_parsers_[infix_type](std::move(left));
    }
    return left;
}

// --- 工具和错误恢复 ---
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