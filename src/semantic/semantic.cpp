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

void define_builtin_functions(SymbolTable &symbol_table) {
    // printInt(n: i32) -> ()
    std::vector<std::shared_ptr<Type>> printInt_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto printInt_return_type = std::make_shared<UnitType>();
    auto printInt_type = std::make_shared<FunctionType>(printInt_return_type, printInt_param_types);

    auto printInt_symbol = std::make_shared<Symbol>("printInt", Symbol::FUNCTION, printInt_type);
    printInt_symbol->is_builtin = true;

    symbol_table.define("printInt", printInt_symbol);

    //  getInt() -> i32
    auto getInt_type = std::make_shared<FunctionType>(
        std::make_shared<PrimitiveType>(TypeKind::I32), std::vector<std::shared_ptr<Type>>{});
    auto getInt_symbol = std::make_shared<Symbol>("getInt", Symbol::FUNCTION, getInt_type);
    getInt_symbol->is_builtin = true;
    symbol_table.define("getInt", getInt_symbol);
}

void Semantic(std::shared_ptr<Program> &ast, ErrorReporter &error_reporter) {

    NameResolutionVisitor name_resolver(error_reporter);
    SymbolTable &global_symbol_table_name = name_resolver.get_global_symbol_table();

    define_builtin_functions(global_symbol_table_name);

    global_symbol_table_name.enter_scope();
    for (auto &item : ast->items) {
        item->accept(&name_resolver);
    }
    if (error_reporter.has_errors()) {
        std::cerr << "Name resolution completed with errors." << std::endl;
        return;
    }

    global_symbol_table_name.exit_scope();

    TypeCheckVisitor type_checker(error_reporter);

    for (auto &item : ast->items) {
        item->accept(&type_checker);
    }
    if (error_reporter.has_errors()) {
        std::cerr << "Type checking completed with errors." << std::endl;
        return;
    }
}