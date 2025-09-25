// parser.cpp
#include "parser.h"

#include <iostream>
#include <memory>

using std::string;
using std::vector;

// Constructor: set up all parsing rules
Parser::Parser(const std::vector<Token> &tokens, ErrorReporter &error_reporter)
    : tokens_(tokens), error_reporter_(error_reporter) {
    // Register Pratt parser rules

    // Register prefix parsing functions
    register_prefix(TokenType::IDENTIFIER, [this] -> std::shared_ptr<Expr> {
        if (previous().lexeme == "_") {
            return std::make_shared<UnderscoreExpr>(previous());
        } else {
            return std::make_shared<VariableExpr>(previous());
        }
    });
    register_prefix(TokenType::NUMBER,
                    [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::STRING,
                    [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::RSTRING,
                    [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::CSTRING,
                    [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::RCSTRING,
                    [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::CHAR, [this] { return std::make_shared<LiteralExpr>(previous()); });

    register_prefix(TokenType::TRUE, [this] { return std::make_shared<LiteralExpr>(previous()); });
    register_prefix(TokenType::FALSE, [this] { return std::make_shared<LiteralExpr>(previous()); });

    register_prefix(TokenType::MINUS, [this] {
        auto op = previous();
        auto right = parse_expression(Precedence::UNARY);
        return std::make_shared<UnaryExpr>(op, std::move(right));
    });
    register_prefix(TokenType::BANG, [this] {
        auto op = previous();
        auto right = parse_expression(Precedence::UNARY);
        return std::make_shared<UnaryExpr>(op, std::move(right));
    });
    register_prefix(TokenType::AMPERSAND, [this]() -> std::shared_ptr<Expr> {
        bool is_mutable = match({TokenType::MUT});
        auto expr = parse_expression(Precedence::UNARY);
        return std::make_shared<ReferenceExpr>(is_mutable, std::move(expr));
    });
    register_prefix(TokenType::STAR, [this] {
        Token op = previous();
        auto right = parse_expression(Precedence::UNARY);
        return std::make_shared<UnaryExpr>(op, std::move(right));
    });
    register_prefix(TokenType::IF, [this] { return parse_if_expression(); });
    register_prefix(TokenType::WHILE, [this] { return parse_while_expression(); });
    register_prefix(TokenType::LOOP, [this] { return parse_loop_expression(); });
    register_prefix(TokenType::MATCH, [this] { return parse_match_expression(); });
    register_prefix(TokenType::RETURN, [this] { return parse_return_expression(); });

    register_prefix(TokenType::LEFT_PAREN, [this] -> std::shared_ptr<Expr> {
        if (check(TokenType::RIGHT_PAREN)) {
            consume(TokenType::RIGHT_PAREN, "Unclosed unit literal.");
            return std::make_shared<UnitExpr>();
        }
        auto expr = parse_expression(Precedence::NONE);
        if (match({TokenType::COMMA})) {
            std::vector<std::shared_ptr<Expr>> elements;
            elements.push_back(std::move(expr));

            while (!check(TokenType::RIGHT_PAREN) && !is_at_end()) {
                elements.push_back(parse_expression(Precedence::NONE));
                if (!check(TokenType::RIGHT_PAREN)) {
                    consume(TokenType::COMMA, "Expect ',' between tuple elements.");
                }
            }
            consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple.");
            return std::make_shared<TupleExpr>(std::move(elements));
        } else {

            consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
            return std::make_shared<GroupingExpr>(std::move(expr));
        }
    });

    register_prefix(TokenType::LEFT_BRACKET, [this] -> std::shared_ptr<Expr> {
        if (check(TokenType::RIGHT_BRACKET)) {
            Token closing = consume(TokenType::RIGHT_BRACKET, "Unclosed empty array literal.");
            return std::make_shared<ArrayLiteralExpr>(std::vector<std::shared_ptr<Expr>>{});
        }
        auto first_expr = parse_expression(Precedence::NONE);
        if (match({TokenType::SEMICOLON})) {
            auto count_expr = parse_expression(Precedence::NONE);
            consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array initializer expression.");
            return std::make_shared<ArrayInitializerExpr>(std::move(first_expr),
                                                          std::move(count_expr));
        } else {
            std::vector<std::shared_ptr<Expr>> elements;
            elements.push_back(std::move(first_expr));
            while (match({TokenType::COMMA})) {
                if (check(TokenType::RIGHT_BRACKET)) {
                    break;
                }
                elements.push_back(parse_expression(Precedence::NONE));
            }
            Token closing = consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array literal.");
            return std::make_shared<ArrayLiteralExpr>(std::move(elements));
        }
    });

    register_prefix(TokenType::SELF_TYPE,
                    [this] { return std::make_shared<VariableExpr>(previous()); });
    register_prefix(TokenType::SELF, [this] { return std::make_shared<VariableExpr>(previous()); });

    register_prefix(TokenType::LEFT_BRACE, [this] {
        current_--;
        auto block = parse_block_statement();

        return std::make_shared<BlockExpr>(std::move(block));
    });

    register_infix(TokenType::AS, Precedence::AS, [this](auto left) -> std::shared_ptr<Expr> {
        auto target_type = parse_type();
        if (!target_type) {
            report_error(peek(), "Expect a type after 'as' keyword.");
            return nullptr;
        }
        return std::make_shared<AsExpr>(std::move(left), std::move(target_type));
    });

    // Register infix parsing functions and precedence
    register_infix(TokenType::PLUS, Precedence::TERM, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::TERM);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::MINUS, Precedence::TERM, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::TERM);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::STAR, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::SLASH, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PERCENT, Precedence::FACTOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::FACTOR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND, Precedence::BITWISE_AND, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_AND);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE, Precedence::BITWISE_OR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_OR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::CARET, Precedence::BITWISE_XOR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::BITWISE_XOR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_LESS, Precedence::SHIFT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::SHIFT);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::GREATER_GREATER, Precedence::SHIFT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::SHIFT);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::EQUAL_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::BANG_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER_EQUAL, Precedence::COMPARISON, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::COMPARISON);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND_AMPERSAND, Precedence::AND, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::AND);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE_PIPE, Precedence::OR, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::OR);
        return std::make_shared<BinaryExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::LEFT_PAREN, Precedence::CALL, [this](auto callee) {
        std::vector<std::shared_ptr<Expr>> arguments;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                if (peek().type == TokenType::RIGHT_PAREN)
                    break;
                arguments.push_back(parse_expression(Precedence::NONE));
            } while (match({TokenType::COMMA}));
        }
        Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
        return std::make_shared<CallExpr>(std::move(callee), std::move(arguments));
    });

    register_infix(TokenType::DOT, Precedence::CALL, [this](auto left) {
        Token field_name = consume(TokenType::IDENTIFIER, "Expect field name after '.'.");
        return std::make_shared<FieldAccessExpr>(std::move(left), field_name);
    });

    register_infix(TokenType::LEFT_BRACKET, Precedence::CALL, [this](auto left) {
        auto index_expr = parse_expression(Precedence::NONE);

        Token closing_bracket = consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
        return std::make_shared<IndexExpr>(std::move(left), std::move(index_expr));
    });
    register_infix(TokenType::EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto value = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<AssignmentExpr>(std::move(left), std::move(value));
    });
    register_infix(TokenType::PLUS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::MINUS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::STAR_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::SLASH_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PERCENT_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::AMPERSAND_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::PIPE_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::CARET_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::LESS_LESS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::GREATER_GREATER_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
        auto op = previous();
        auto right = parse_expression(Precedence::ASSIGNMENT);
        return std::make_shared<CompoundAssignmentExpr>(std::move(left), op, std::move(right));
    });
    register_infix(TokenType::COLON_COLON, Precedence::PATH, [this](auto left) {
        Token op = previous();
        auto right_token = consume(TokenType::IDENTIFIER, "Expect identifier after '::'.");
        auto right = std::make_shared<VariableExpr>(right_token);
        return std::make_shared<PathExpr>(std::move(left), op, std::move(right));
    });

    register_infix(TokenType::LEFT_BRACE, Precedence::CALL, [this](std::shared_ptr<Expr> left) {
        current_--;
        return parse_struct_initializer(std::move(left));
    });
}

// Main parsing loop
std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    while (!is_at_end()) {
        auto item = parse_item();
        if (item) {
            program->items.push_back(item);
        }
    }
    return program;
}

std::shared_ptr<TypeNode> Parser::parse_type() {

    if (match({TokenType::BANG})) {
        return std::make_shared<TypeNameNode>(previous());
    }
    if (match({TokenType::SELF_TYPE})) {
        return std::make_shared<SelfTypeNode>();
    }

    if (match({TokenType::STAR})) {
        bool is_mutable = match({TokenType::MUT});
        if (!is_mutable) {
            consume(TokenType::CONST, "Expect 'const' or 'mut' after '*' in raw pointer type.");
        }
        auto pointee_type = parse_type();
        return std::make_shared<RawPointerTypeNode>(is_mutable, std::move(pointee_type));
    }
    if (match({TokenType::AMPERSAND})) {
        bool is_mutable = match({TokenType::MUT});
        auto referenced_type = parse_type();
        return std::make_shared<ReferenceTypeNode>(is_mutable, std::move(referenced_type));
    }
    if (match({TokenType::LEFT_BRACKET})) {
        auto element_type = parse_type();
        if (match({TokenType::SEMICOLON})) {
            auto size = parse_expression(Precedence::NONE);
            consume(TokenType::RIGHT_BRACKET, "Expect ']' to close array type.");
            return std::make_shared<ArrayTypeNode>(std::move(element_type), std::move(size));
        } else {
            consume(TokenType::RIGHT_BRACKET, "Expect ']' to close slice type.");
            return std::make_shared<SliceTypeNode>(std::move(element_type));
        }
    }

    if (match({TokenType::LEFT_PAREN})) {
        if (match({TokenType::RIGHT_PAREN})) {
            return std::make_shared<UnitTypeNode>();
        } else if (check(TokenType::IDENTIFIER)) {
            std::vector<std::shared_ptr<TypeNode>> elements;
            do {
                elements.push_back(parse_type());
            } while (match({TokenType::COMMA}));
            consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple type.");
            return std::make_shared<TupleTypeNode>(std::move(elements));
        }
    }
    auto path = parse_path_expression();
    if (match({TokenType::LESS})) {
        std::vector<std::shared_ptr<TypeNode>> args;
        if (!check(TokenType::GREATER)) {
            do {
                args.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::GREATER, "Expect '>' to close generic argument list.");
        return std::make_shared<PathTypeNode>(std::move(path), std::move(args));
    } else {
        return std::make_shared<PathTypeNode>(std::move(path));
    }

    report_error(peek(), "Expected a type.");
    return nullptr;
}

// pattern

std::shared_ptr<Pattern> Parser::parse_pattern() {
    if (match({TokenType::AMPERSAND})) {
        bool is_mutable = match({TokenType::MUT});
        auto pattern = parse_pattern();
        return std::make_shared<ReferencePattern>(is_mutable, std::move(pattern));
    }
    if (match({TokenType::LEFT_PAREN})) {
        std::vector<std::shared_ptr<Pattern>> elements;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                elements.push_back(parse_pattern());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple pattern.");
        return std::make_shared<TuplePattern>(std::move(elements));
    }

    if (match({TokenType::LEFT_BRACKET})) {
        std::vector<std::shared_ptr<Pattern>> elements;
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                if (peek().type == TokenType::DOT_DOT) {
                    advance();
                    elements.push_back(std::make_shared<RestPattern>());
                    match({TokenType::COMMA});
                    break;
                }
                elements.push_back(parse_pattern());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_BRACKET, "Expect ']' to close slice pattern.");
        return std::make_shared<SlicePattern>(std::move(elements));
    }

    bool is_mutable = match({TokenType::MUT});

    if (match({TokenType::IDENTIFIER})) {
        if (previous().lexeme == "_") {
            return std::make_shared<WildcardPattern>();
        }
        if (peek().type == TokenType::LEFT_BRACE) {
            return parse_struct_pattern_body(std::make_shared<VariableExpr>(previous()));
        } else {
            return std::make_shared<IdentifierPattern>(previous(), is_mutable);
        }
    }

    if (match({TokenType::NUMBER, TokenType::STRING, TokenType::TRUE, TokenType::FALSE})) {
        return std::make_shared<LiteralPattern>(previous());
    }

    report_error(peek(), "Expected a pattern.");
    return nullptr;
}

std::shared_ptr<Pattern> Parser::parse_struct_pattern_body(std::shared_ptr<Expr> path) {
    consume(TokenType::LEFT_BRACE, "Expect '{' to start struct pattern.");

    std::vector<std::shared_ptr<StructPatternField>> fields;
    bool has_rest = false;

    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {

        if (match({TokenType::DOT, TokenType::DOT})) {
            has_rest = true;
            break;
        }
        Token field_name = consume(TokenType::IDENTIFIER, "Expect field name in struct pattern.");
        std::optional<std::shared_ptr<Pattern>> pattern;

        if (match({TokenType::COLON})) {
            pattern = parse_pattern();
        } else {
            pattern = std::nullopt;
        }
        fields.push_back(std::make_shared<StructPatternField>(field_name, std::move(pattern)));
        if (!check(TokenType::RIGHT_BRACE)) {
            consume(TokenType::COMMA, "Expect ',' after field in struct pattern.");
        }
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' to close struct pattern.");

    return std::make_shared<StructPattern>(std::move(path), std::move(fields), has_rest);
}

// Recursive descent implementation
std::shared_ptr<Item> Parser::parse_item() {

    if (peek().type == TokenType::FN) {
        return parse_fn_declaration();
    } else if (peek().type == TokenType::STRUCT) {
        return parse_struct_declaration();
    } else if (peek().type == TokenType::CONST) {
        return parse_const_declaration();
    } else if (peek().type == TokenType::ENUM) {
        return parse_enum_declaration();
    } else if (peek().type == TokenType::MOD) {
        return parse_mod_declaration();
    } else if (peek().type == TokenType::TRAIT) {
        return parse_trait_declaration();
    } else if (peek().type == TokenType::IMPL) {
        return parse_impl_block();
    }

    report_error(peek(), "Expect a top-level item like 'fn'.");
    advance();
    return nullptr;
}

std::shared_ptr<FnDecl> Parser::parse_fn_declaration() {
    consume(TokenType::FN, "Expect 'fn'.");
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");
    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");

    std::vector<std::shared_ptr<FnParam>> params;
    std::optional<std::shared_ptr<TypeNode>> return_type;

    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            std::shared_ptr<Pattern> pattern;
            std::shared_ptr<TypeNode> type;

            if (peek().type == TokenType::AMPERSAND && peekNext().type == TokenType::SELF) {
                advance();
                bool is_mutable = match({TokenType::MUT});
                Token self_token = consume(TokenType::SELF, "Expect 'self' after '&' in receiver.");

                pattern = std::make_shared<IdentifierPattern>(self_token, false);
                auto self_type_token = Token{TokenType::SELF_TYPE, "Self", 0, 0};
                auto self_type_node =
                    std::make_shared<PathTypeNode>(std::make_shared<VariableExpr>(self_type_token));
                type = std::make_shared<ReferenceTypeNode>(is_mutable, std::move(self_type_node));
            } else if (peek().type == TokenType::SELF && peekNext().type != TokenType::COLON) {
                Token self_token = consume(TokenType::SELF, "Expect 'self' parameter.");
                pattern = std::make_shared<IdentifierPattern>(self_token, false);
                auto self_type_token = Token{TokenType::SELF_TYPE, "Self", 0, 0};
                type =
                    std::make_shared<PathTypeNode>(std::make_shared<VariableExpr>(self_type_token));
            } else {
                pattern = parse_pattern();
                consume(TokenType::COLON, "Expect ':' after parameter pattern.");
                type = parse_type();
            }

            params.push_back(std::make_shared<FnParam>(std::move(pattern), std::move(type)));

        } while (match({TokenType::COMMA}));
    }

    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    if (check(TokenType::ARROW)) {
        advance();
        return_type = parse_type();
    }
    std::optional<std::shared_ptr<BlockStmt>> body;
    if (peek().type == TokenType::LEFT_BRACE) {
        body = parse_block_statement();
    } else if (match({TokenType::SEMICOLON})) {
    } else {
        report_error(peek(), "Expect function body `{` or semicolon `;` after function signature.");
        return nullptr;
    }
    return std::make_shared<FnDecl>(name, std::move(params), std::move(return_type),
                                    std::move(body));
}

std::shared_ptr<StructDecl> Parser::parse_struct_declaration() {
    consume(TokenType::STRUCT, "Expect 'struct' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect struct name.");

    if (peek().type == TokenType::LEFT_BRACE) {
        consume(TokenType::LEFT_BRACE, "Expect '{' before struct body.");
        std::vector<std::shared_ptr<Field>> fields;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
            consume(TokenType::COLON, "Expect ':' after field name.");
            auto field_type = parse_type();
            fields.push_back(std::make_shared<Field>(field_name, std::move(field_type)));
            if (!match({TokenType::COMMA}))
                break;
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' after struct body.");
        return std::make_shared<StructDecl>(name, std::move(fields));

    } else if (peek().type == TokenType::LEFT_PAREN) {
        consume(TokenType::LEFT_PAREN, "Expect '(' for tuple struct.");
        std::vector<std::shared_ptr<TypeNode>> tuple_fields;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                tuple_fields.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after tuple struct fields.");
        consume(TokenType::SEMICOLON, "Expect ';' after tuple struct declaration.");
        return std::make_shared<StructDecl>(name, std::move(tuple_fields));

    } else {
        consume(TokenType::SEMICOLON, "Expect ';' for unit-like struct declaration.");
        return std::make_shared<StructDecl>(name);
    }
}

std::shared_ptr<Expr> Parser::parse_struct_initializer(std::shared_ptr<Expr> name) {
    consume(TokenType::LEFT_BRACE, "Expect '{' for struct initializer.");
    std::vector<std::shared_ptr<FieldInitializer>> fields;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        Token field_name = advance();
        if (field_name.type != TokenType::IDENTIFIER && field_name.type != TokenType::NUMBER) {
            report_error(field_name, "Expect field name or index in struct initializer.");
            return nullptr;
        }

        consume(TokenType::COLON, "Expect ':' after field name.");
        auto value = parse_expression(Precedence::NONE);

        fields.push_back(std::make_shared<FieldInitializer>(field_name, std::move(value)));
        if (!check(TokenType::RIGHT_BRACE)) {
            consume(TokenType::COMMA, "Expect ',' after field value.");
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to close struct initializer.");

    return std::make_shared<StructInitializerExpr>(std::move(name), std::move(fields));
}
std::shared_ptr<ConstDecl> Parser::parse_const_declaration() {
    consume(TokenType::CONST, "Expect 'const' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect constant name.");
    consume(TokenType::COLON, "Expect ':' after constant name.");
    auto type = parse_type();
    if (!type) {
        report_error(peek(), "Expect a type for the constant.");
        return nullptr;
    }
    consume(TokenType::EQUAL, "Expect '=' after constant type.");
    auto value = parse_expression(Precedence::NONE);
    consume(TokenType::SEMICOLON, "Expect ';' after constant value.");
    return std::make_shared<ConstDecl>(name, std::move(type), std::move(value));
}
std::shared_ptr<EnumDecl> Parser::parse_enum_declaration() {
    consume(TokenType::ENUM, "Expect 'enum' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect enum name.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before enum body.");

    std::vector<std::shared_ptr<EnumVariant>> variants;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        variants.push_back(parse_enum_variant());

        if (!check(TokenType::RIGHT_BRACE)) {
            consume(TokenType::COMMA, "Expect ',' after enum variant.");
        }
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after enum body.");
    return std::make_shared<EnumDecl>(name, std::move(variants));
}

std::shared_ptr<EnumVariant> Parser::parse_enum_variant() {
    Token name = consume(TokenType::IDENTIFIER, "Expect variant name.");
    if (peek().type == TokenType::LEFT_BRACE) {
        advance();

        std::vector<std::shared_ptr<Field>> fields;
        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
                consume(TokenType::COLON, "Expect ':' after field name.");
                auto field_type = parse_type();
                fields.push_back(std::make_shared<Field>(field_name, std::move(field_type)));
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RIGHT_BRACE, "Expect '}' after struct variant fields.");
        return std::make_shared<EnumVariant>(name, std::move(fields));
    } else if (peek().type == TokenType::LEFT_PAREN) {
        advance();

        std::vector<std::shared_ptr<TypeNode>> tuple_types;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                tuple_types.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RIGHT_PAREN, "Expect ')' after tuple variant types.");
        return std::make_shared<EnumVariant>(name, std::move(tuple_types));
    } else {
        std::optional<std::shared_ptr<Expr>> discriminant;
        if (match({TokenType::EQUAL})) {
            discriminant = parse_expression(Precedence::NONE);
        }
        return std::make_shared<EnumVariant>(name, std::move(discriminant));
    }
}

std::shared_ptr<ModDecl> Parser::parse_mod_declaration() {
    consume(TokenType::MOD, "Expect 'mod' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect module name.");

    if (match({TokenType::LEFT_BRACE})) {
        std::vector<std::shared_ptr<Item>> items;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            items.push_back(parse_item());
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' to close module body.");
        return std::make_shared<ModDecl>(name, std::move(items));
    } else if (match({TokenType::SEMICOLON})) {
        std::vector<std::shared_ptr<Item>> items;
        return std::make_shared<ModDecl>(name, std::move(items));
    } else {
        report_error(peek(), "Expect '{' or ';' after module name.");
        return nullptr;
    }
}

std::shared_ptr<TraitDecl> Parser::parse_trait_declaration() {
    consume(TokenType::TRAIT, "Expect 'trait' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect trait name.");
    consume(TokenType::LEFT_BRACE, "Expect '{' before trait body.");
    std::vector<std::shared_ptr<Item>> items;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        if (peek().type == TokenType::FN) {
            items.push_back(parse_fn_declaration());
        } else {
            report_error(peek(), "Expect associated function, type, or const in trait body.");
            advance(); // Skip the invalid token to continue parsing
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after trait body.");
    return std::make_shared<TraitDecl>(name, std::move(items));
}

std::shared_ptr<ImplBlock> Parser::parse_impl_block() {
    consume(TokenType::IMPL, "Expect 'impl' keyword.");

    std::optional<std::shared_ptr<TypeNode>> trait_name;
    std::shared_ptr<TypeNode> target_type;
    auto first_type = parse_type();
    if (match({TokenType::FOR})) {
        trait_name = std::move(first_type);
        target_type = parse_type();
    } else {
        target_type = std::move(first_type);
    }

    consume(TokenType::LEFT_BRACE, "Expect '{' before impl body.");

    std::vector<std::shared_ptr<Item>> items;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        if (peek().type == TokenType::FN) {
            items.push_back(parse_fn_declaration());
        } else {
            report_error(peek(), "Expect associated function, type, or const in impl body.");
            advance(); // Skip the invalid token to continue parsing
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after impl body.");
    return std::make_shared<ImplBlock>(std::move(trait_name), std::move(target_type),
                                       std::move(items));
}

// statement
std::shared_ptr<Stmt> Parser::parse_statement() {
    if (peek().type == TokenType::LET)
        return parse_let_statement();
    if (peek().type == TokenType::RETURN)
        return parse_return_statement();
    if (peek().type == TokenType::FN) {
        return std::make_shared<ItemStmt>(parse_fn_declaration());
    }
    if (peek().type == TokenType::STRUCT) {
        return std::make_shared<ItemStmt>(parse_struct_declaration());
    }
    if (peek().type == TokenType::CONST) {
        return std::make_shared<ItemStmt>(parse_const_declaration());
    }
    if (peek().type == TokenType::ENUM) {
        return std::make_shared<ItemStmt>(parse_enum_declaration());
    }
    if (peek().type == TokenType::MOD) {
        return std::make_shared<ItemStmt>(parse_mod_declaration());
    }
    if (peek().type == TokenType::TRAIT) {
        return std::make_shared<ItemStmt>(parse_trait_declaration());
    }
    if (peek().type == TokenType::IMPL) {
        return std::make_shared<ItemStmt>(parse_impl_block());
    }
    if (peek().type == TokenType::BREAK) {
        return parse_break_statement();
    }
    if (peek().type == TokenType::CONTINUE) {
        return parse_continue_statement();
    }

    return parse_expression_statement();
}

std::shared_ptr<BlockStmt> Parser::parse_block_statement() {
    consume(TokenType::LEFT_BRACE, "Expect '{' to start a block.");
    auto block = std::make_shared<BlockStmt>();
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        block->statements.push_back(parse_statement());
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to end a block.");

    block->has_semicolon = 1;
    if (!block->statements.empty()) {
        if (auto *last_stmt = dynamic_cast<ExprStmt *>(block->statements.back().get())) {
            if (!last_stmt->has_semicolon) {
                block->final_expr = std::move(last_stmt->expression);
                block->statements.pop_back();
                block->has_semicolon = 0;
            }
        }
        for (size_t i = 0; i + 1 < block->statements.size(); i++) {
            if (auto *last_stmt = dynamic_cast<ExprStmt *>(block->statements[i].get())) {
                if (!last_stmt->has_semicolon) {

                    report_error(
                        peek(), "Only the final expression in a block can be without a semicolon.");
                    break;
                }
            }
        }
    }
    return block;
}
std::shared_ptr<LetStmt> Parser::parse_let_statement() {
    consume(TokenType::LET, "Expect 'let'.");

    auto pattern = parse_pattern();

    std::optional<std::shared_ptr<TypeNode>> type_annotation;
    if (match({TokenType::COLON})) {
        type_annotation = parse_type();
    }
    std::optional<std::shared_ptr<Expr>> initializer;
    if (match({TokenType::EQUAL})) {
        initializer = parse_expression(Precedence::NONE);
    }

    consume(TokenType::SEMICOLON, "Expect ';' after let statement.");
    return std::make_shared<LetStmt>(std::move(pattern), std::move(type_annotation),
                                     std::move(initializer));
}

std::shared_ptr<ReturnStmt> Parser::parse_return_statement() {
    Token keyword = consume(TokenType::RETURN, "Expect 'return'.");
    std::optional<std::shared_ptr<Expr>> value;
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression(Precedence::NONE);
    }
    if (peek().type == TokenType::SEMICOLON) {
        consume(TokenType::SEMICOLON, "Expect ';' after return statement.");
    } else if (!is_at_end() && peek().type != TokenType::RIGHT_BRACE) {
        report_error(peek(), "Expect ';' after return statement.");
    }

    if (peek().type == TokenType::SEMICOLON) {
        puts("a");
    }
    return std::make_shared<ReturnStmt>(keyword, std::move(value));
}

std::shared_ptr<BreakStmt> Parser::parse_break_statement() {
    Token keyword = consume(TokenType::BREAK, "Expect 'break'.");
    std::optional<std::shared_ptr<Expr>> value;
    if (!check(TokenType::SEMICOLON) && !is_at_end()) {
        value = parse_expression(Precedence::NONE);
    }
    if (peek().type == TokenType::RIGHT_BRACE) {
        return std::make_shared<BreakStmt>(std::move(value));
    }
    consume(TokenType::SEMICOLON, "Expect ';' after break statement.");
    return std::make_shared<BreakStmt>(std::move(value));
}

std::shared_ptr<ContinueStmt> Parser::parse_continue_statement() {
    Token keyword = consume(TokenType::CONTINUE, "Expect 'continue'.");
    consume(TokenType::SEMICOLON, "Expect ';' after continue statement.");
    return std::make_shared<ContinueStmt>();
}

std::shared_ptr<ExprStmt> Parser::parse_expression_statement() {
    auto expr = parse_expression(Precedence::NONE);
    if (match({TokenType::SEMICOLON})) {
        return std::make_shared<ExprStmt>(std::move(expr), true);
    } else {
        if (auto *if_expr = dynamic_cast<IfExpr *>(expr.get())) {
            return std::make_shared<ExprStmt>(std::move(expr), if_expr->has_semicolon);
        } else if (auto *match_expr = dynamic_cast<MatchExpr *>(expr.get())) {
            return std::make_shared<ExprStmt>(std::move(expr), true);
        } else if (auto *loop_expr = dynamic_cast<LoopExpr *>(expr.get())) {
            return std::make_shared<ExprStmt>(std::move(expr), true);
        } else if (auto *while_expr = dynamic_cast<WhileExpr *>(expr.get())) {
            return std::make_shared<ExprStmt>(std::move(expr), true);
        }
        return std::make_shared<ExprStmt>(std::move(expr), false);
    }
    return std::make_shared<ExprStmt>(std::move(expr), true);
}

Precedence Parser::get_precedence(TokenType type) {
    if (precedences_.count(type)) {
        return precedences_[type];
    }
    return Precedence::NONE;
}

std::shared_ptr<Expr> Parser::parse_expression(Precedence precedence) {
    advance();
    TokenType prefix_type = previous().type;
    if (prefix_parsers_.find(prefix_type) == prefix_parsers_.end()) {
        report_error(previous(), "Expect an expression.");
        return nullptr;
    }

    auto left = prefix_parsers_[prefix_type]();

    while (precedence < get_precedence(peek().type)) {
        if (auto *if_expr = dynamic_cast<IfExpr *>(left.get())) {
            if (!if_expr->else_branch || if_expr->has_semicolon) {
                break;
            }
        }
        advance();
        TokenType infix_type = previous().type;
        if (infix_parsers_.find(infix_type) == infix_parsers_.end()) {
            return left;
        }
        left = infix_parsers_[infix_type](std::move(left));
    }

    return left;
}

std::shared_ptr<Expr> Parser::parse_return_expression() {
    Token keyword = previous();
    std::optional<std::shared_ptr<Expr>> value;
    if (!check(TokenType::SEMICOLON)) {
        value = parse_expression(Precedence::NONE);
    }
    return std::make_shared<ReturnExpr>(std::make_shared<ReturnStmt>(keyword, std::move(value)));
}

std::shared_ptr<BlockExpr> Parser::parse_block_expression() {
    auto block_stmt = parse_block_statement();

    return std::make_shared<BlockExpr>(std::move(block_stmt));
}

std::shared_ptr<IfExpr> Parser::parse_if_expression() {

    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    auto condition = parse_expression(Precedence::NONE);
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
    bool has_semicolon = false;

    auto then_branch = parse_block_expression();
    has_semicolon = then_branch->block_stmt->has_semicolon;
    std::optional<std::shared_ptr<Expr>> else_branch;
    if (match({TokenType::ELSE})) {
        else_branch = parse_expression(Precedence::NONE);
        has_semicolon |= else_branch && else_branch.value()->has_semicolon;
    }

    return std::make_shared<IfExpr>(std::move(condition), std::move(then_branch),
                                    std::move(else_branch), has_semicolon);
}

std::shared_ptr<LoopExpr> Parser::parse_loop_expression() {
    auto body = parse_block_statement();
    return std::make_shared<LoopExpr>(std::move(body));
}

std::shared_ptr<WhileExpr> Parser::parse_while_expression() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'.");

    auto condition = parse_expression(Precedence::NONE);

    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");

    auto body = parse_block_statement();

    return std::make_shared<WhileExpr>(std::move(condition), std::move(body));
}

std::shared_ptr<MatchArm> Parser::parse_match_arm() {
    auto pattern = parse_pattern();
    std::optional<std::shared_ptr<Expr>> guard;
    if (match({TokenType::IF})) {
        guard = parse_expression(Precedence::NONE);
    }

    consume(TokenType::FAT_ARROW, "Expect '=>' after match arm pattern.");
    auto body = parse_expression(Precedence::NONE);
    if (!check(TokenType::RIGHT_BRACE)) {
        consume(TokenType::COMMA, "Expect ',' after match arm body.");
    }
    return std::make_shared<MatchArm>(std::move(pattern), std::move(guard), std::move(body));
}

std::shared_ptr<MatchExpr> Parser::parse_match_expression() {
    auto scrutinee = parse_expression(Precedence::NONE);

    consume(TokenType::LEFT_BRACE, "Expect '{' after match scrutinee.");

    std::vector<std::shared_ptr<MatchArm>> arms;
    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        arms.push_back(parse_match_arm());
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' to close match expression.");

    return std::make_shared<MatchExpr>(std::move(scrutinee), std::move(arms));
}

std::shared_ptr<Expr> Parser::parse_path_expression() {
    if (!check(TokenType::IDENTIFIER)) {
        report_error(peek(), "Expected a path-like identifier for a type.");
    }

    std::shared_ptr<Expr> path = std::make_shared<VariableExpr>(advance());
    while (match({TokenType::COLON_COLON})) {
        Token op = previous();
        Token right_token =
            consume(TokenType::IDENTIFIER, "Expect identifier after '::' in a type path.");
        auto right = std::make_shared<VariableExpr>(right_token);
        path = std::make_shared<PathExpr>(std::move(path), op, std::move(right));
    }

    return path;
}

// tools
bool Parser::is_at_end() {
    return current_ >= tokens_.size() || peek().type == TokenType::END_OF_FILE;
}
const Token &Parser::peek() { return tokens_[current_]; }
const Token &Parser::peekNext() {
    if (is_at_end()) {
        static Token unknown_token{TokenType::UNKNOWN, "nullptr", 0, 0};
        return unknown_token;
    }
    return tokens_[current_ + 1];
}
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
    report_error(peek(), error_message);
    return peek(); // Return current token to avoid crash
}
void Parser::report_error(const Token &token, const std::string &message) {
    error_reporter_.report_error(message, token.line, token.column);
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

// Implementation of registration functions
void Parser::register_prefix(TokenType type, PrefixParseFn fn) { prefix_parsers_[type] = fn; }
void Parser::register_infix(TokenType type, Precedence prec, InfixParseFn fn) {
    infix_parsers_[type] = fn;
    precedences_[type] = prec;
}