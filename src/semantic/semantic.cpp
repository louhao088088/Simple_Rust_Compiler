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
    symbol_table.define("i32",
                        std::make_shared<Symbol>("i32", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::I32)));
    symbol_table.define("u32",
                        std::make_shared<Symbol>("u32", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::U32)));
    symbol_table.define("isize",
                        std::make_shared<Symbol>("isize", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::ISIZE)));
    symbol_table.define("usize",
                        std::make_shared<Symbol>("usize", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::USIZE)));
    symbol_table.define(
        "anyint", std::make_shared<Symbol>("anyint", Symbol::TYPE,
                                           std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER)));
    symbol_table.define("bool",
                        std::make_shared<Symbol>("bool", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::BOOL)));
    symbol_table.define("char",
                        std::make_shared<Symbol>("char", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::CHAR)));
    symbol_table.define("str",
                        std::make_shared<Symbol>("str", Symbol::TYPE,
                                                 std::make_shared<PrimitiveType>(TypeKind::STR)));
    symbol_table.define(
        "String", std::make_shared<Symbol>("String", Symbol::TYPE,
                                           std::make_shared<PrimitiveType>(TypeKind::STRING)));
    symbol_table.define(
        "rstring", std::make_shared<Symbol>("rstring", Symbol::TYPE,
                                            std::make_shared<PrimitiveType>(TypeKind::RSTRING)));
    symbol_table.define(
        "cstring", std::make_shared<Symbol>("cstring", Symbol::TYPE,
                                            std::make_shared<PrimitiveType>(TypeKind::CSTRING)));
    symbol_table.define(
        "rcstring", std::make_shared<Symbol>("rcstring", Symbol::TYPE,
                                             std::make_shared<PrimitiveType>(TypeKind::RCSTRING)));

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

void define_builtin_method(SymbolTable &symbol_table) {
    auto u32_symbol = symbol_table.lookup("u32");
    auto string_symbol = symbol_table.lookup("String");
    auto str_symbol = symbol_table.lookup("str");
    auto usize_symbol = symbol_table.lookup("usize");
    auto anyint_symbol = symbol_table.lookup("anyint");

    if (!u32_symbol || !string_symbol || !str_symbol || !usize_symbol || !anyint_symbol) {
        std::cerr << "FATAL: A required built-in type was not defined." << std::endl;
        return;
    }

    auto u32_type = u32_symbol->type;
    auto usize_type = usize_symbol->type;
    auto string_type = string_symbol->type;
    auto str_type = str_symbol->type;
    auto anyint_type = anyint_symbol->type;

    // u32 methods
    {
        // fn to_string(&self) -> String
        std::vector<std::shared_ptr<Type>> to_string_param_types = {
            std::make_shared<ReferenceType>(u32_type)};
        auto to_string_return_type = string_type;
        auto to_string_type =
            std::make_shared<FunctionType>(to_string_return_type, to_string_param_types);

        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        u32_type->members->define("to_string", to_string_symbol);
    }
    // usize methods
    {
        // fn to_string(&self) -> String
        std::vector<std::shared_ptr<Type>> to_string_param_types = {
            std::make_shared<ReferenceType>(usize_type)};
        auto to_string_return_type = string_type;
        auto to_string_type =
            std::make_shared<FunctionType>(to_string_return_type, to_string_param_types);

        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        usize_type->members->define("to_string", to_string_symbol);
    }

    // anyint methods
    {
        // fn to_string(&self) -> String
        std::vector<std::shared_ptr<Type>> to_string_param_types = {std::make_shared<ReferenceType>(
            std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER))};
        auto to_string_return_type = string_type;
        auto to_string_type =
            std::make_shared<FunctionType>(to_string_return_type, to_string_param_types);

        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        anyint_type->members->define("to_string", to_string_symbol);
    }

    // String methods
    {
        // fn len(&self) -> usize
        std::vector<std::shared_ptr<Type>> len_param_types = {
            std::make_shared<ReferenceType>(str_type)};
        auto len_return_type = usize_type;
        auto len_type = std::make_shared<FunctionType>(len_return_type, len_param_types);

        auto len_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, len_type);
        len_symbol->is_builtin = true;
        str_type->members->define("len", len_symbol);

        // impl String {
        // fn as_str(&self) -> &str
        // fn as_mut_str(&mut self) -> &mut str
        //}

        std::vector<std::shared_ptr<Type>> as_str_param_types = {
            std::make_shared<ReferenceType>(string_type)};
        auto as_str_return_type = std::make_shared<ReferenceType>(str_type);
        auto as_str_type = std::make_shared<FunctionType>(as_str_return_type, as_str_param_types);

        auto as_str_symbol = std::make_shared<Symbol>("as_str", Symbol::FUNCTION, as_str_type);
        as_str_symbol->is_builtin = true;
        string_type->members->define("as_str", as_str_symbol);

        std::vector<std::shared_ptr<Type>> as_mut_str_param_types = {
            std::make_shared<ReferenceType>(string_type, true)};
        auto as_mut_str_return_type = std::make_shared<ReferenceType>(str_type, true);
        auto as_mut_str_type =
            std::make_shared<FunctionType>(as_mut_str_return_type, as_mut_str_param_types);
        auto as_mut_str_symbol =
            std::make_shared<Symbol>("as_mut_str", Symbol::FUNCTION, as_mut_str_type);
        as_mut_str_symbol->is_builtin = true;
        string_type->members->define("as_mut_str", as_mut_str_symbol);
    }
    //&str, &mut str methods
    {
        // fn len(&self) -> usize

        auto ref_str_type =
            std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR));
        std::vector<std::shared_ptr<Type>> len_param_types = {
            std::make_shared<ReferenceType>(ref_str_type)};
        auto len_return_type = usize_type;
        auto len_type = std::make_shared<FunctionType>(len_return_type, len_param_types);
        auto len_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, len_type);
        len_symbol->is_builtin = true;
        ref_str_type->members->define("len", len_symbol);

        auto mut_ref_str_type =
            std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR), true);
        std::vector<std::shared_ptr<Type>> mut_len_param_types = {
            std::make_shared<ReferenceType>(mut_ref_str_type)};
        auto mut_len_return_type = usize_type;
        auto mut_len_type =
            std::make_shared<FunctionType>(mut_len_return_type, mut_len_param_types);
        auto mut_len_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, mut_len_type);
        mut_len_symbol->is_builtin = true;
        mut_ref_str_type->members->define("len", mut_len_symbol);
    }

    // String
    {
        auto string_symbol = symbol_table.lookup("String");
        if (string_symbol) {

            // from(&str) -> String
            std::vector<std::shared_ptr<Type>> from_params;
            from_params.push_back(std::make_shared<ReferenceType>(str_type, false));
            auto from_type = std::make_shared<FunctionType>(string_type, from_params);
            auto from_symbol = std::make_shared<Symbol>("from", Symbol::FUNCTION, from_type);
            from_symbol->is_builtin = true;
            string_symbol->members->define("from", from_symbol);

            // from(&mut str) -> String
            from_params.clear();
            from_params.push_back(std::make_shared<ReferenceType>(str_type, true));
            from_type = std::make_shared<FunctionType>(string_type, from_params);
            from_symbol = std::make_shared<Symbol>("from", Symbol::FUNCTION, from_type);
            from_symbol->is_builtin = true;
            string_symbol->members->define("from", from_symbol);
        }

        // fn append(&mut self, s: &str) -> ()
        if (string_symbol) {
            std::vector<std::shared_ptr<Type>> append_params;
            append_params.push_back(std::make_shared<ReferenceType>(string_type, true));
            append_params.push_back(std::make_shared<ReferenceType>(str_type, false));
            auto append_type =
                std::make_shared<FunctionType>(std::make_shared<UnitType>(), append_params);
            auto append_symbol = std::make_shared<Symbol>("append", Symbol::FUNCTION, append_type);
            append_symbol->is_builtin = true;
            string_symbol->members->define("append", append_symbol);
        }
    }

    //
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
    define_builtin_method(global_symbol_table_name);

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

std::optional<std::string> get_name_from_expr(Expr *expr) {
    if (!expr) {
        return std::nullopt;
    }
    if (auto *var_expr = dynamic_cast<VariableExpr *>(expr)) {
        return var_expr->name.lexeme;
    }
    return std::nullopt;
}