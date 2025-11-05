#include "semantic.h"
std::optional<long long> ConstEvaluator::visit(LiteralExpr *node) {
    if (node->literal.type == TokenType::NUMBER) {

        return number_of_tokens(node->literal.lexeme, error_reporter_).value;
    }
    return std::nullopt;
}

std::optional<long long> ConstEvaluator::visit(VariableExpr *node) {
    std::shared_ptr<Symbol> symbol = node->resolved_symbol;

    if (symbol && symbol->kind == Symbol::CONSTANT && symbol->const_decl_node) {
        return this->evaluate(symbol->const_decl_node->value.get());
    }

    return std::nullopt;
}

std::optional<long long> ConstEvaluator::visit(BinaryExpr *node) {
    auto left_val = evaluate(node->left.get());
    auto right_val = evaluate(node->right.get());

    if (left_val && right_val) {
        switch (node->op.type) {
        case TokenType::PLUS:
            return *left_val + *right_val;
        case TokenType::MINUS:
            return *left_val - *right_val;
        case TokenType::STAR:
            return *left_val * *right_val;
        case TokenType::SLASH: {
            if (*right_val == 0) {
                error_reporter_.report_error("Division by zero in constant expression.",
                                             node->op.line);
                return std::nullopt;
            }
            return *left_val / *right_val;
        }
        case TokenType::EQUAL_EQUAL:
            return (*left_val == *right_val) ? 1 : 0;
        case TokenType::BANG_EQUAL:
            return (*left_val != *right_val) ? 1 : 0;
        case TokenType::LESS:
            return (*left_val < *right_val) ? 1 : 0;
        case TokenType::LESS_EQUAL:
            return (*left_val <= *right_val) ? 1 : 0;
        case TokenType::GREATER:
            return (*left_val > *right_val) ? 1 : 0;
        case TokenType::GREATER_EQUAL:
            return (*left_val >= *right_val) ? 1 : 0;
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<long long> ConstEvaluator::visit(AsExpr *node) {

    auto left_value_opt = evaluate(node->expression.get());

    if (!left_value_opt) {
        return std::nullopt;
    }

    if (node->target_type && node->target_type->resolved_type) {
        auto target_type_kind = node->target_type->resolved_type->kind;
        if (!is_concrete_integer(target_type_kind) && target_type_kind != TypeKind::ANY_INTEGER) {
            error_reporter_.report_error("Constant casting is only supported for integer types.");
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
    return left_value_opt;
}

std::optional<long long> ConstEvaluator::visit(UnaryExpr *node) {
    auto operand_val = evaluate(node->right.get());

    if (operand_val) {
        switch (node->op.type) {
        case TokenType::MINUS:
            return -*operand_val;
        case TokenType::PLUS:
            return *operand_val;
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<long long> ConstEvaluator::visit(GroupingExpr *node) {
    return evaluate(node->expression.get());
}