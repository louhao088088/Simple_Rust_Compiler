// semantic.cpp

#include "semantic.h"


// SymbolTable implementation

void SymbolTable::enter_scope() { scopes_.emplace_back(); }

void SymbolTable::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

bool SymbolTable::define(const std::string &name, std::shared_ptr<Symbol> symbol) {
    if (!scopes_.empty()) {
        scopes_.back()[name] = symbol;
        return true;
    }
    return false;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string &name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}






std::optional<long long> ConstEvaluator::visit(LiteralExpr *node) {
    if (node->literal.type == TokenType::NUMBER) {

        return number_of_tokens(node->literal.lexeme, error_reporter_).value;
    }
    return std::nullopt;
}

std::optional<long long> ConstEvaluator::visit(VariableExpr *node) { return std::nullopt; }

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
        case TokenType::SLASH:
            return *left_val / *right_val;
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
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