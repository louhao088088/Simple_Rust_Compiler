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

std::optional<long long> ConstEvaluator::visit(VariableExpr *node) {
    std::shared_ptr<Symbol> symbol = node->resolved_symbol;

    if (!symbol) {
        symbol = symbol_table_.lookup(node->name.lexeme);
    }

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

void define_builtin_functions(SymbolTable &symbol_table) {

    // print(s: &str) -> ()
    std::vector<std::shared_ptr<Type>> print_param_types = {
        std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR))};
    auto print_return_type = std::make_shared<UnitType>();
    auto print_type = std::make_shared<FunctionType>(print_return_type, print_param_types);

    auto print_symbol = std::make_shared<Symbol>("print", Symbol::FUNCTION, print_type);
    print_symbol->is_builtin = true;
    symbol_table.define("print", print_symbol);

    // println(s: &str) -> ()
    std::vector<std::shared_ptr<Type>> println_param_types = {
        std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR))};
    auto println_return_type = std::make_shared<UnitType>();
    auto println_type = std::make_shared<FunctionType>(println_return_type, println_param_types);

    auto println_symbol = std::make_shared<Symbol>("println", Symbol::FUNCTION, println_type);
    println_symbol->is_builtin = true;
    symbol_table.define("println", println_symbol);

    // printInt(n: i32) -> ()
    std::vector<std::shared_ptr<Type>> printInt_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto printInt_return_type = std::make_shared<UnitType>();
    auto printInt_type = std::make_shared<FunctionType>(printInt_return_type, printInt_param_types);

    auto printInt_symbol = std::make_shared<Symbol>("printInt", Symbol::FUNCTION, printInt_type);
    printInt_symbol->is_builtin = true;

    symbol_table.define("printInt", printInt_symbol);

    // printlnInt(n: i32) -> ()
    std::vector<std::shared_ptr<Type>> printlnInt_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto printlnInt_return_type = std::make_shared<UnitType>();
    auto printlnInt_type =
        std::make_shared<FunctionType>(printlnInt_return_type, printlnInt_param_types);

    auto printlnInt_symbol =
        std::make_shared<Symbol>("printlnInt", Symbol::FUNCTION, printlnInt_type);
    printlnInt_symbol->is_builtin = true;

    symbol_table.define("printlnInt", printlnInt_symbol);

    //  getInt() -> i32
    auto getInt_type = std::make_shared<FunctionType>(
        std::make_shared<PrimitiveType>(TypeKind::I32), std::vector<std::shared_ptr<Type>>{});
    auto getInt_symbol = std::make_shared<Symbol>("getInt", Symbol::FUNCTION, getInt_type);
    getInt_symbol->is_builtin = true;
    symbol_table.define("getInt", getInt_symbol);

    // getString() -> String
    auto getString_type = std::make_shared<FunctionType>(
        std::make_shared<PrimitiveType>(TypeKind::STRING), std::vector<std::shared_ptr<Type>>{});
    auto getString_symbol = std::make_shared<Symbol>("getString", Symbol::FUNCTION, getString_type);
    getString_symbol->is_builtin = true;
    symbol_table.define("getString", getString_symbol);

    // exit(code: i32) -> ()
    std::vector<std::shared_ptr<Type>> exit_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto exit_return_type = std::make_shared<UnitType>();
    auto exit_type = std::make_shared<FunctionType>(exit_return_type, exit_param_types);

    auto exit_symbol = std::make_shared<Symbol>("exit", Symbol::FUNCTION, exit_type);
    exit_symbol->is_builtin = true;
    symbol_table.define("exit", exit_symbol);
}

bool is_concrete_integer(TypeKind kind) {
    return kind == TypeKind::I32 || kind == TypeKind::U32 || kind == TypeKind::ISIZE ||
           kind == TypeKind::USIZE;
}

bool is_any_integer_type(TypeKind kind) {
    return is_concrete_integer(kind) || kind == TypeKind::ANY_INTEGER;
}

void Semantic(std::shared_ptr<Program> &ast, ErrorReporter &error_reporter) {

    NameResolutionVisitor name_resolver(error_reporter);
    SymbolTable &global_symbol_table_name = name_resolver.get_global_symbol_table();

    define_builtin_functions(global_symbol_table_name);

    global_symbol_table_name.enter_scope();

    name_resolver.resolve(ast.get());

    if (error_reporter.has_errors()) {
        std::cerr << "Name resolution completed with errors." << std::endl;
        return;
    } else {
        std::cerr << "Name resolution completed successfully." << std::endl;
    }

    TypeCheckVisitor type_checker(global_symbol_table_name, error_reporter);

    for (auto &item : ast->items) {
        item->accept(&type_checker);
    }
    if (error_reporter.has_errors()) {
        std::cerr << "Type checking completed with errors." << std::endl;
        return;
    }

    global_symbol_table_name.exit_scope();
}