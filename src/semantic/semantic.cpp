// semantic.cpp

#include "semantic.h"

void SymbolTable::enter_scope() { scopes_.emplace_back(); }

void SymbolTable::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

bool SymbolTable::define_value(const std::string &name, std::shared_ptr<Symbol> symbol) {
    if (scopes_.empty()) {
        return false;
    }

    auto &current_scope_map = scopes_.back().value_symbols;

    if (current_scope_map.find(name) != current_scope_map.end()) {
        return false;
    }

    current_scope_map.insert({name, symbol});
    return true;
}

bool SymbolTable::define_variable(const std::string &name, std::shared_ptr<Symbol> symbol,
                                  bool allow_shadow) {
    if (scopes_.empty())
        return false;
    auto &scope = scopes_.back();
    auto it = scope.value_symbols.find(name);
    if (it != scope.value_symbols.end()) {
        if (!allow_shadow) {
            return false;
        }
    }
    scope.value_symbols[name] = symbol;
    return true;
}

bool SymbolTable::define_type(const std::string &name, std::shared_ptr<Symbol> symbol) {
    if (scopes_.empty())
        return false;
    auto &scope = scopes_.back();
    if (scope.type_symbols.find(name) != scope.type_symbols.end()) {
        return false;
    }
    scope.type_symbols[name] = symbol;
    return true;
}

std::shared_ptr<Symbol> SymbolTable::lookup_value(const std::string &name) {

    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        const auto &scope_map = it->value_symbols;
        auto found_it = scope_map.find(name);
        if (found_it != scope_map.end()) {
            return found_it->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Symbol> SymbolTable::lookup_type(const std::string &name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->type_symbols.find(name);
        if (found != it->type_symbols.end()) {
            return found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::define(const std::string &name, std::shared_ptr<Symbol> symbol) {
    if (!symbol)
        return false;
    switch (symbol->kind) {
    case Symbol::TYPE:
        return define_type(name, symbol);
    case Symbol::FUNCTION:
    case Symbol::VARIABLE:
    case Symbol::MODULE:
    case Symbol::VARIANT:
    case Symbol::CONSTANT:
        return define_value(name, symbol);
    }
    return false;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string &name) {
    auto v = lookup_value(name);
    if (v)
        return v;
    return lookup_type(name);
}

void define_builtin_functions(SymbolTable &symbol_table, BuiltinTypes &builtin_types) {
    symbol_table.define_type(
        "i32", std::make_shared<Symbol>("i32", Symbol::TYPE,
                                        std::make_shared<PrimitiveType>(TypeKind::I32)));
    symbol_table.define_type(
        "u32", std::make_shared<Symbol>("u32", Symbol::TYPE,
                                        std::make_shared<PrimitiveType>(TypeKind::U32)));
    symbol_table.define_type(
        "isize", std::make_shared<Symbol>("isize", Symbol::TYPE,
                                          std::make_shared<PrimitiveType>(TypeKind::ISIZE)));
    symbol_table.define_type(
        "usize", std::make_shared<Symbol>("usize", Symbol::TYPE,
                                          std::make_shared<PrimitiveType>(TypeKind::USIZE)));
    symbol_table.define_type(
        "anyint", std::make_shared<Symbol>("anyint", Symbol::TYPE,
                                           std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER)));
    symbol_table.define_type(
        "bool", std::make_shared<Symbol>("bool", Symbol::TYPE,
                                         std::make_shared<PrimitiveType>(TypeKind::BOOL)));
    symbol_table.define_type(
        "char", std::make_shared<Symbol>("char", Symbol::TYPE,
                                         std::make_shared<PrimitiveType>(TypeKind::CHAR)));
    symbol_table.define_type(
        "str", std::make_shared<Symbol>("str", Symbol::TYPE,
                                        std::make_shared<PrimitiveType>(TypeKind::STR)));
    symbol_table.define_type(
        "String", std::make_shared<Symbol>("String", Symbol::TYPE,
                                           std::make_shared<PrimitiveType>(TypeKind::STRING)));
    symbol_table.define_type(
        "rstring", std::make_shared<Symbol>("rstring", Symbol::TYPE,
                                            std::make_shared<PrimitiveType>(TypeKind::RSTRING)));
    symbol_table.define_type(
        "cstring", std::make_shared<Symbol>("cstring", Symbol::TYPE,
                                            std::make_shared<PrimitiveType>(TypeKind::CSTRING)));
    symbol_table.define_type(
        "rcstring", std::make_shared<Symbol>("rcstring", Symbol::TYPE,
                                             std::make_shared<PrimitiveType>(TypeKind::RCSTRING)));

    // print(s: &str) -> ()
    std::vector<std::shared_ptr<Type>> print_param_types = {
        std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR))};
    auto print_return_type = std::make_shared<UnitType>();
    auto print_type = std::make_shared<FunctionType>(print_return_type, print_param_types);

    auto print_symbol = std::make_shared<Symbol>("print", Symbol::FUNCTION, print_type);
    print_symbol->is_builtin = true;
    symbol_table.define_value("print", print_symbol);

    // println(s: &str) -> ()
    std::vector<std::shared_ptr<Type>> println_param_types = {
        std::make_shared<ReferenceType>(std::make_shared<PrimitiveType>(TypeKind::STR))};
    auto println_return_type = std::make_shared<UnitType>();
    auto println_type = std::make_shared<FunctionType>(println_return_type, println_param_types);

    auto println_symbol = std::make_shared<Symbol>("println", Symbol::FUNCTION, println_type);
    println_symbol->is_builtin = true;
    symbol_table.define_value("println", println_symbol);

    // printInt(n: i32) -> ()
    std::vector<std::shared_ptr<Type>> printInt_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto printInt_return_type = std::make_shared<UnitType>();
    auto printInt_type = std::make_shared<FunctionType>(printInt_return_type, printInt_param_types);

    auto printInt_symbol = std::make_shared<Symbol>("printInt", Symbol::FUNCTION, printInt_type);
    printInt_symbol->is_builtin = true;

    symbol_table.define_value("printInt", printInt_symbol);

    // printlnInt(n: i32) -> ()
    std::vector<std::shared_ptr<Type>> printlnInt_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto printlnInt_return_type = std::make_shared<UnitType>();
    auto printlnInt_type =
        std::make_shared<FunctionType>(printlnInt_return_type, printlnInt_param_types);

    auto printlnInt_symbol =
        std::make_shared<Symbol>("printlnInt", Symbol::FUNCTION, printlnInt_type);
    printlnInt_symbol->is_builtin = true;

    symbol_table.define_value("printlnInt", printlnInt_symbol);

    //  getInt() -> i32
    auto getInt_type = std::make_shared<FunctionType>(
        std::make_shared<PrimitiveType>(TypeKind::I32), std::vector<std::shared_ptr<Type>>{});
    auto getInt_symbol = std::make_shared<Symbol>("getInt", Symbol::FUNCTION, getInt_type);
    getInt_symbol->is_builtin = true;
    symbol_table.define_value("getInt", getInt_symbol);

    // getString() -> String
    auto getString_type = std::make_shared<FunctionType>(
        std::make_shared<PrimitiveType>(TypeKind::STRING), std::vector<std::shared_ptr<Type>>{});
    auto getString_symbol = std::make_shared<Symbol>("getString", Symbol::FUNCTION, getString_type);
    getString_symbol->is_builtin = true;
    symbol_table.define_value("getString", getString_symbol);

    // exit(code: i32) -> ()
    std::vector<std::shared_ptr<Type>> exit_param_types = {
        std::make_shared<PrimitiveType>(TypeKind::I32)};
    auto exit_return_type = std::make_shared<UnitType>();
    auto exit_type = std::make_shared<FunctionType>(exit_return_type, exit_param_types);

    auto exit_symbol = std::make_shared<Symbol>("exit", Symbol::FUNCTION, exit_type);
    exit_symbol->is_builtin = true;
    symbol_table.define_value("exit", exit_symbol);
}

void define_builtin_method(SymbolTable &symbol_table, BuiltinTypes &builtin_types) {
    auto u32_symbol = symbol_table.lookup_type("u32");
    auto string_symbol = symbol_table.lookup_type("String");
    auto str_symbol = symbol_table.lookup_type("str");
    auto usize_symbol = symbol_table.lookup_type("usize");
    auto anyint_symbol = symbol_table.lookup_type("anyint");

    builtin_types.u32_type = u32_symbol ? u32_symbol->type : nullptr;
    builtin_types.i32_type = symbol_table.lookup_type("i32")->type;
    builtin_types.isize_type = symbol_table.lookup_type("isize")->type;
    builtin_types.usize_type = usize_symbol ? usize_symbol->type : nullptr;
    builtin_types.string_type = string_symbol ? string_symbol->type : nullptr;
    builtin_types.str_type = str_symbol ? str_symbol->type : nullptr;
    builtin_types.bool_type = symbol_table.lookup_type("bool")->type;
    builtin_types.char_type = symbol_table.lookup_type("char")->type;
    builtin_types.any_integer_type = anyint_symbol ? anyint_symbol->type : nullptr;

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
        std::vector<std::shared_ptr<Type>> to_string_param_types = {
            std::make_shared<ReferenceType>(u32_type)};
        auto to_string_type = std::make_shared<FunctionType>(string_type, to_string_param_types);
        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        u32_type->members->define_value("to_string", to_string_symbol);
    }
    // usize methods
    {
        std::vector<std::shared_ptr<Type>> to_string_param_types = {
            std::make_shared<ReferenceType>(usize_type)};
        auto to_string_type = std::make_shared<FunctionType>(string_type, to_string_param_types);
        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        usize_type->members->define_value("to_string", to_string_symbol);
    }
    // anyint methods
    {
        std::vector<std::shared_ptr<Type>> to_string_param_types = {std::make_shared<ReferenceType>(
            std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER))};
        auto to_string_type = std::make_shared<FunctionType>(string_type, to_string_param_types);
        auto to_string_symbol =
            std::make_shared<Symbol>("to_string", Symbol::FUNCTION, to_string_type);
        to_string_symbol->is_builtin = true;
        anyint_type->members->define_value("to_string", to_string_symbol);
    }
    // str methods (len)
    {
        std::vector<std::shared_ptr<Type>> len_param_types = {
            std::make_shared<ReferenceType>(str_type)};
        auto len_type = std::make_shared<FunctionType>(usize_type, len_param_types);
        auto len_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, len_type);
        len_symbol->is_builtin = true;
        str_type->members->define_value("len", len_symbol);
    }
    // String methods
    {
        // as_str(&self) -> &str
        std::vector<std::shared_ptr<Type>> as_str_params = {
            std::make_shared<ReferenceType>(string_type)};
        auto as_str_ret = std::make_shared<ReferenceType>(str_type);
        auto as_str_type = std::make_shared<FunctionType>(as_str_ret, as_str_params);
        auto as_str_symbol = std::make_shared<Symbol>("as_str", Symbol::FUNCTION, as_str_type);
        as_str_symbol->is_builtin = true;
        string_type->members->define_value("as_str", as_str_symbol);

        // as_mut_str(&mut self) -> &mut str
        std::vector<std::shared_ptr<Type>> as_mut_str_params = {
            std::make_shared<ReferenceType>(string_type, true)};
        auto as_mut_str_ret = std::make_shared<ReferenceType>(str_type, true);
        auto as_mut_str_type = std::make_shared<FunctionType>(as_mut_str_ret, as_mut_str_params);
        auto as_mut_str_symbol =
            std::make_shared<Symbol>("as_mut_str", Symbol::FUNCTION, as_mut_str_type);
        as_mut_str_symbol->is_builtin = true;
        string_type->members->define_value("as_mut_str", as_mut_str_symbol);

        // from(&str) -> String
        if (string_symbol) {
            std::vector<std::shared_ptr<Type>> from_params = {
                std::make_shared<ReferenceType>(str_type)};
            auto from_type = std::make_shared<FunctionType>(string_type, from_params);
            auto from_symbol = std::make_shared<Symbol>("from", Symbol::FUNCTION, from_type);
            from_symbol->is_builtin = true;
            string_symbol->members->define_value("from", from_symbol);
        }

        // append(&mut self, s: &str) -> ()
        std::vector<std::shared_ptr<Type>> append_params = {
            std::make_shared<ReferenceType>(string_type, true),
            std::make_shared<ReferenceType>(str_type)};
        auto append_type =
            std::make_shared<FunctionType>(std::make_shared<UnitType>(), append_params);
        auto append_symbol = std::make_shared<Symbol>("append", Symbol::FUNCTION, append_type);
        append_symbol->is_builtin = true;
        string_type->members->define_value("append", append_symbol);
    }
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
    SymbolTable &symbol_table = name_resolver.get_global_symbol_table();
    BuiltinTypes builtins;

    define_builtin_functions(symbol_table, builtins);
    define_builtin_method(symbol_table, builtins);

    symbol_table.enter_scope();

    name_resolver.resolve(ast.get());

    if (error_reporter.has_errors()) {
        std::cerr << "Name resolution completed with errors." << std::endl;
        return;
    } else {
        std::cerr << "Name resolution completed successfully." << std::endl;
    }

    TypeCheckVisitor type_checker(symbol_table, builtins, error_reporter);

    for (auto &item : ast->items) {
        item->accept(&type_checker);
    }
    if (error_reporter.has_errors()) {
        std::cerr << "Type checking completed with errors." << std::endl;
        return;
    }

    symbol_table.exit_scope();
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