// semantic.cpp

#include "semantic.h"

#include "../tool/number.h"

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

// NameResolutionVisitor implementation
NameResolutionVisitor::NameResolutionVisitor(ErrorReporter &error_reporter)
    : error_reporter_(error_reporter), type_resolver_(*this, symbol_table_, error_reporter_) {
    symbol_table_.enter_scope();
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(LiteralExpr *node) { return nullptr; }

std::shared_ptr<Symbol> NameResolutionVisitor::visit(ArrayLiteralExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(ArrayInitializerExpr *node) {
    node->value->accept(this);
    node->size->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(VariableExpr *node) {
    auto symbol = symbol_table_.lookup(node->name.lexeme);
    if (!symbol) {
        error_reporter_.report_error("Undefined variable '" + node->name.lexeme + "'",
                                     node->name.line, node->name.column);
        return nullptr;
    } else {
        node->resolved_symbol = symbol;
        return symbol;
    }
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(UnaryExpr *node) {
    node->right->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(CallExpr *node) {
    node->callee->accept(this);
    for (auto &arg : node->arguments) {
        arg->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(IfExpr *node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(LoopExpr *node) {
    node->body->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(WhileExpr *node) {
    node->condition->accept(this);
    node->body->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(IndexExpr *node) {
    node->object->accept(this);
    node->index->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(CompoundAssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(ReferenceExpr *node) {
    node->expression->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(UnderscoreExpr *node) { return nullptr; }

void NameResolutionVisitor::visit(BlockStmt *node) {
    symbol_table_.enter_scope();
    for (auto &stmt : node->statements) {
        stmt->accept(this);
    }
    if (node->final_expr) {
        (*node->final_expr)->accept(this);
    }
    symbol_table_.exit_scope();
}

void NameResolutionVisitor::visit(ExprStmt *node) { node->expression->accept(this); }

void NameResolutionVisitor::visit(LetStmt *node) {
    std::shared_ptr<Type> var_type = nullptr;
    if (node->type_annotation) {
        var_type = type_resolver_.resolve(node->type_annotation->get());
        if (!var_type) {
            error_reporter_.report_error("Cannot resolve type annotation for variable.");
        }
    } else {
        error_reporter_.report_error("Variable declaration must have a type annotation.");
    }
    if (node->initializer) {
        (*node->initializer)->accept(this);
    }

    current_let_type_ = var_type;
    node->pattern->accept(this);
    current_let_type_ = nullptr;
}

void NameResolutionVisitor::visit(ReturnStmt *node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void NameResolutionVisitor::visit(BreakStmt *node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void NameResolutionVisitor::visit(ContinueStmt *node) {}

void NameResolutionVisitor::visit(TypeNameNode *node) {}

void NameResolutionVisitor::visit(ArrayTypeNode *node) {
    node->element_type->accept(this);
    node->size->accept(this);
}

void NameResolutionVisitor::visit(UnitTypeNode *node) {}

void NameResolutionVisitor::visit(TupleTypeNode *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(FnDecl *node) {
    auto fn_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::FUNCTION);
    symbol_table_.define(node->name.lexeme, fn_symbol);
    node->resolved_symbol = fn_symbol;

    if (node->body) {
        symbol_table_.enter_scope();
        for (const auto &param : node->params) {
            // Handle FnParam manually since it's not an Expr
            param->pattern->accept(this);
            if (param->type)
                param->type->accept(this);
        }
        (*node->body)->accept(this);
        symbol_table_.exit_scope();
    }
}

// Pattern visitors
void NameResolutionVisitor::visit(IdentifierPattern *node) {
    auto var_symbol =
        std::make_shared<Symbol>(node->name.lexeme, Symbol::VARIABLE, current_let_type_);

    if (!symbol_table_.define(node->name.lexeme, var_symbol)) {
        error_reporter_.report_error("Variable '" + node->name.lexeme +
                                         "' is already defined in this scope.",
                                     node->name.line);
    }

    node->resolved_symbol = var_symbol;
}

void NameResolutionVisitor::visit(WildcardPattern *node) {}

void NameResolutionVisitor::visit(LiteralPattern *node) {}

void NameResolutionVisitor::visit(TuplePattern *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(SlicePattern *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(StructPattern *node) {}

void NameResolutionVisitor::visit(RestPattern *node) {}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(StructInitializerExpr *node) {
    node->name->accept(this);
    for (auto &field : node->fields) {
        field->value->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(UnitExpr *node) { return nullptr; }

std::shared_ptr<Symbol> NameResolutionVisitor::visit(GroupingExpr *node) {
    return node->expression->accept(this);
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(TupleExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(AsExpr *node) {
    node->expression->accept(this);
    node->target_type->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(MatchExpr *node) {
    node->scrutinee->accept(this);
    for (auto &arm : node->arms) {
        // Visit components of MatchArm manually since it's not an Expr
        arm->pattern->accept(this);
        if (arm->guard)
            (*arm->guard)->accept(this);
        arm->body->accept(this);
    }
    return nullptr;
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

std::string get_full_path_string(Expr *expr) {
    if (!expr)
        return "";

    if (auto *var = dynamic_cast<VariableExpr *>(expr)) {
        return var->name.lexeme;
    }
    if (auto *path = dynamic_cast<PathExpr *>(expr)) {
        auto right_name = get_name_from_expr(path->right.get());
        return get_full_path_string(path->left.get()) + "::" + (right_name ? *right_name : "?");
    }

    return "<complex_expression>";
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(PathExpr *node) {
    auto left_symbol = node->left->accept(this);

    if (!left_symbol) {
        return nullptr;
    }

    if (!left_symbol->members) {
        std::string left_path_str = get_full_path_string(node->left.get());
        error_reporter_.report_error("'" + left_path_str + "' is not a module or enum");
        return nullptr;
    }

    auto right_name_opt = get_name_from_expr(node->right.get());
    if (!right_name_opt) {
        error_reporter_.report_error("Invalid right-hand side of a '::' path");
        return nullptr;
    }
    std::string right_name = *right_name_opt;

    auto final_symbol = left_symbol->members->lookup(right_name);

    if (!final_symbol) {
        error_reporter_.report_error("name '" + right_name + "' is not found in '" +
                                     left_symbol->name + "'");
        return nullptr;
    }

    node->resolved_symbol = final_symbol;
    return final_symbol;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(BlockExpr *node) {
    node->block_stmt->accept(this);
    return nullptr;
}

void NameResolutionVisitor::visit(ItemStmt *node) { node->item->accept(this); }

void NameResolutionVisitor::visit(PathTypeNode *node) { node->path->accept(this); }

void NameResolutionVisitor::visit(RawPointerTypeNode *node) { node->pointee_type->accept(this); }

void NameResolutionVisitor::visit(ReferenceTypeNode *node) { node->referenced_type->accept(this); }

void NameResolutionVisitor::visit(SliceTypeNode *node) { node->element_type->accept(this); }

void NameResolutionVisitor::visit(SelfTypeNode *node) {}

void NameResolutionVisitor::visit(StructDecl *node) {

    auto struct_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::TYPE);
    auto struct_type =
        std::make_shared<StructType>(node->name.lexeme, std::weak_ptr<Symbol>(struct_symbol));
    struct_symbol->type = struct_type;

    if (!symbol_table_.define(node->name.lexeme, struct_symbol)) {
        error_reporter_.report_error("Type '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
        return;
    }
    node->resolved_symbol = struct_symbol;

    switch (node->kind) {
    case StructKind::Normal: {
        for (const auto &field_node : node->fields) {
            std::shared_ptr<Type> field_type = type_resolver_.resolve(field_node->type.get());
            if (!field_type) {
                error_reporter_.report_error("Unknown type for field '" + field_node->name.lexeme +
                                                 "'.",
                                             field_node->name.line);
                continue;
            }
            auto field_symbol =
                std::make_shared<Symbol>(field_node->name.lexeme, Symbol::VARIABLE, field_type);

            if (!struct_symbol->members->define(field_node->name.lexeme, field_symbol)) {
                error_reporter_.report_error("Field '" + field_node->name.lexeme +
                                                 "' is already defined in struct '" +
                                                 node->name.lexeme + "'.",
                                             field_node->name.line);
            }

            struct_type->fields[field_node->name.lexeme] = field_type;
        }
        break;
    }

    case StructKind::Unit: {
        break;
    }

    case StructKind::Tuple: {

        for (const auto &field_node : node->fields) {
            std::shared_ptr<Type> field_type = type_resolver_.resolve(field_node->type.get());
            if (!field_type) {
                error_reporter_.report_error("Unknown type for tuple field.",
                                             field_node->name.line);
                continue;
            }

            struct_type->fields[std::to_string(struct_type->fields.size())] = field_type;
        }
        break;
    }
    }
}

void NameResolutionVisitor::visit(ConstDecl *node) {
    auto const_type = type_resolver_.resolve(node->type.get());

    if (!const_type) {
        error_reporter_.report_error("Unknown type used in const declaration.", node->name.line,
                                     node->name.column);
        return;
    }

    node->value->accept(this);
    auto const_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::CONSTANT, const_type);

    const_symbol->const_decl_node = node;

    if (!symbol_table_.define(node->name.lexeme, const_symbol)) {
        error_reporter_.report_error("Constant '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
    }
    node->resolved_symbol = const_symbol;

    node->value->accept(this);
}

void NameResolutionVisitor::visit(EnumDecl *node) {
    auto enum_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::TYPE);

    enum_symbol->members = std::make_unique<SymbolTable>();

    symbol_table_.define(node->name.lexeme, enum_symbol);
    node->resolved_symbol = enum_symbol;

    for (const auto &variant : node->variants) {
        auto variant_symbol = std::make_shared<Symbol>(variant->name.lexeme, Symbol::VARIANT);
        enum_symbol->members->define(variant->name.lexeme, variant_symbol);
    }
}

void NameResolutionVisitor::visit(ModDecl *node) {
    auto mod_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::MODULE);
    mod_symbol->members = std::make_unique<SymbolTable>();

    if (!symbol_table_.define(node->name.lexeme, mod_symbol)) {
        error_reporter_.report_error("Module '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
    }
    node->resolved_symbol = mod_symbol;
    symbol_table_.enter_scope();
    for (auto &item : node->items) {
        item->accept(this);
    }
    symbol_table_.exit_scope();
}

void NameResolutionVisitor::visit(TraitDecl *node) {
    for (auto &item : node->associated_items) {
        item->accept(this);
    }
}

void NameResolutionVisitor::visit(ImplBlock *node) {
    if (node->trait_name)
        (*node->trait_name)->accept(this);
    node->target_type->accept(this);

    symbol_table_.enter_scope();

    auto target_type_symbol = node->target_type->resolved_symbol;
    if (target_type_symbol) {
        auto self_symbol = std::make_shared<Symbol>("Self", Symbol::TYPE);
        self_symbol->aliased_symbol = target_type_symbol;
        symbol_table_.define("Self", self_symbol);
    }

    for (auto &item : node->implemented_items) {
        item->accept(this);
    }

    symbol_table_.exit_scope();
}

// TypeCheckVisitor implementation

TypeCheckVisitor::TypeCheckVisitor(ErrorReporter &error_reporter)
    : error_reporter_(error_reporter) {}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(LiteralExpr *node) {
    const TokenType token_type = node->literal.type;
    switch (token_type) {
    case TokenType::NUMBER:
        if (true) {
            node->type = std::make_shared<PrimitiveType>(TypeKind::UNSIGNED_INTEGER);
        } else
            node->type = std::make_shared<PrimitiveType>(TypeKind::INTEGER);

        break;

    case TokenType::TRUE:
    case TokenType::FALSE:
        node->type = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        break;

    case TokenType::STRING:
        node->type = std::make_shared<PrimitiveType>(TypeKind::STRING);
        break;

    case TokenType::CHAR:
        node->type = std::make_shared<PrimitiveType>(TypeKind::CHAR);
        break;

    default:
        error_reporter_.report_error("Unknown literal type encountered.", node->literal.line);
        break;
    }
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(ArrayLiteralExpr *node) {
    // TODO: Type check array elements and infer array type
    for (auto &element : node->elements) {
        element->accept(this);
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(ArrayInitializerExpr *node) {
    node->value->accept(this);
    node->size->accept(this);
    // TODO: Type check and infer array type
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(VariableExpr *node) {
    if (node->resolved_symbol && node->resolved_symbol->type) {
        node->type = node->resolved_symbol->type;
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnaryExpr *node) {
    node->right->accept(this);
    // TODO: Type check unary operations
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);

    auto left_type = node->left->type;
    auto right_type = node->right->type;

    if (left_type && right_type && left_type->equals(right_type.get())) {
        node->type = left_type;
    } else {
        error_reporter_.report_error("Type mismatch in binary expression.");
    }

    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(CallExpr *node) {
    node->callee->accept(this);
    for (auto &arg : node->arguments) {
        arg->accept(this);
    }
    // TODO: Type check function call
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(IfExpr *node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
    // TODO: Type check if expression
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(LoopExpr *node) {
    node->body->accept(this);
    // TODO: Loop expressions return ()
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(WhileExpr *node) {
    node->condition->accept(this);
    node->body->accept(this);
    // TODO: While expressions return ()
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(IndexExpr *node) {
    node->object->accept(this);
    node->index->accept(this);
    // TODO: Type check array/index access
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    // TODO: Type check field access
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    // TODO: Type check assignment compatibility
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(CompoundAssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    // TODO: Type check compound assignment compatibility
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(ReferenceExpr *node) {
    node->expression->accept(this);
    // TODO: Handle reference type checking
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnderscoreExpr *node) {
    // Underscore expression has unknown type for now
    return nullptr; // TODO: Implement proper type checking
}

void TypeCheckVisitor::visit(BlockStmt *node) {
    for (auto &stmt : node->statements) {
        stmt->accept(this);
    }
    if (node->final_expr) {
        (*node->final_expr)->accept(this);
    }
}

void TypeCheckVisitor::visit(ExprStmt *node) { node->expression->accept(this); }

void TypeCheckVisitor::visit(LetStmt *node) {
    if (!node->initializer) {
        return;
    }
    (*node->initializer)->accept(this);
    std::shared_ptr<Type> initializer_type = (*node->initializer)->type;

    if (!initializer_type) {
        return;
    }

    if (auto id_pattern = dynamic_cast<IdentifierPattern *>(node->pattern.get())) {
        if (id_pattern->resolved_symbol) {
            std::shared_ptr<Type> declared_type = id_pattern->resolved_symbol->type;

            // 3. 比较类型
            if (!declared_type->equals(initializer_type.get())) {
                error_reporter_.report_error("Mismatched types. Expected '" +
                                             declared_type->to_string() + "' but found '" +
                                             initializer_type->to_string() + "'.");
            }
        }
    }
}

void TypeCheckVisitor::visit(ReturnStmt *node) {
    if (node->value) {
        (*node->value)->accept(this);
        // TODO: Check return type compatibility
    }
}

void TypeCheckVisitor::visit(BreakStmt *node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void TypeCheckVisitor::visit(ContinueStmt *node) {
    // Continue statements don't have types
}

void TypeCheckVisitor::visit(TypeNameNode *node) {
    // Type nodes don't need type checking
}

void TypeCheckVisitor::visit(ArrayTypeNode *node) {
    node->element_type->accept(this);
    node->size->accept(this);
}

void TypeCheckVisitor::visit(UnitTypeNode *node) {
    // Unit type is always valid
}

void TypeCheckVisitor::visit(TupleTypeNode *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(FnDecl *node) {
    if (node->body) {
        // TODO: Set current return type for return statements
        (*node->body)->accept(this);
    }
}

// Pattern visitors for TypeCheckVisitor (simplified implementations)
void TypeCheckVisitor::visit(IdentifierPattern *node) {
    // Pattern type checking would go here
}

void TypeCheckVisitor::visit(WildcardPattern *node) {
    // No type checking needed for wildcard
}

void TypeCheckVisitor::visit(LiteralPattern *node) {
    // Type check literal pattern
}

void TypeCheckVisitor::visit(TuplePattern *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(SlicePattern *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(StructPattern *node) {
    // TODO: Handle struct pattern type checking
}

void TypeCheckVisitor::visit(RestPattern *node) {
    // No specific type checking for rest patterns
}

// Missing expression visitors for TypeCheckVisitor
std::shared_ptr<Symbol> TypeCheckVisitor::visit(StructInitializerExpr *node) {
    node->name->accept(this);
    for (auto &field : node->fields) {
        field->value->accept(this);
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnitExpr *node) {
    // Unit expressions have unit type
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(GroupingExpr *node) {
    return node->expression->accept(this);
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(TupleExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(AsExpr *node) {
    node->expression->accept(this);
    node->target_type->accept(this);
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(MatchExpr *node) {
    node->scrutinee->accept(this);
    for (auto &arm : node->arms) {
        // Visit components of MatchArm manually since it's not an Expr
        arm->pattern->accept(this);
        if (arm->guard)
            (*arm->guard)->accept(this);
        arm->body->accept(this);
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(PathExpr *node) {
    // TODO: Type check path expressions
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(BlockExpr *node) {
    // TODO: Type check path expressions
    return nullptr; // TODO: Implement proper type checking
}

// Missing statement visitors for TypeCheckVisitor
void TypeCheckVisitor::visit(ItemStmt *node) { node->item->accept(this); }

// Missing type node visitors for TypeCheckVisitor
void TypeCheckVisitor::visit(PathTypeNode *node) { node->path->accept(this); }

void TypeCheckVisitor::visit(RawPointerTypeNode *node) { node->pointee_type->accept(this); }

void TypeCheckVisitor::visit(ReferenceTypeNode *node) { node->referenced_type->accept(this); }

void TypeCheckVisitor::visit(SliceTypeNode *node) { node->element_type->accept(this); }

void TypeCheckVisitor::visit(SelfTypeNode *node) {
    // Self types are handled in context
}

// Missing item visitors for TypeCheckVisitor
void TypeCheckVisitor::visit(StructDecl *node) {
    // TODO: Handle struct type checking
}

void TypeCheckVisitor::visit(ConstDecl *node) {
    node->value->accept(this);
    if (node->type)
        node->type->accept(this);
}

void TypeCheckVisitor::visit(EnumDecl *node) {
    for (auto &variant : node->variants) {
        // EnumVariant doesn't have accept method, handle manually if needed
        // variant->name is just an identifier
        // variant->fields would need type checking if present
    }
}

void TypeCheckVisitor::visit(ModDecl *node) {
    for (auto &item : node->items) {
        item->accept(this);
    }
}

void TypeCheckVisitor::visit(TraitDecl *node) {
    for (auto &item : node->associated_items) {
        item->accept(this);
    }
}

void TypeCheckVisitor::visit(ImplBlock *node) {
    if (node->trait_name)
        (*node->trait_name)->accept(this);
    node->target_type->accept(this);
    for (auto &item : node->implemented_items) {
        item->accept(this);
    }
}

// TypeResolver Implementation

TypeResolver::TypeResolver(NameResolutionVisitor &resolver, SymbolTable &symbols,
                           ErrorReporter &reporter)
    : name_resolver_(resolver), symbol_table_(symbols), error_reporter_(reporter) {}

std::shared_ptr<Type> TypeResolver::resolve(TypeNode *node) {
    if (!node)
        return nullptr;

    resolved_type_ = nullptr;
    node->accept(this);
    return resolved_type_;
}

void TypeResolver::visit(TypeNameNode *node) {
    // Handle primitive types
    if (node->name.lexeme == "i32") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::INTEGER);
    } else if (node->name.lexeme == "u32") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::UNSIGNED_INTEGER);
    } else if (node->name.lexeme == "isize") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::INTEGER);
    } else if (node->name.lexeme == "usize") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::UNSIGNED_INTEGER);
    } else if (node->name.lexeme == "bool") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::BOOL);
    } else if (node->name.lexeme == "char") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::CHAR);
    } else if (node->name.lexeme == "string") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::STRING);
    } else {
        auto symbol = symbol_table_.lookup(node->name.lexeme);
        if (symbol && symbol->kind == Symbol::TYPE) {
            resolved_type_ = symbol->type;
            node->resolved_symbol = symbol;
        } else {
            resolved_type_ = nullptr;
        }
    }
}

void TypeResolver::visit(ArrayTypeNode *node) {
    auto element_type = resolve(node->element_type.get());
    if (!element_type) {
        resolved_type_ = nullptr;
        return;
    }

    ConstEvaluator const_evaluator(symbol_table_, error_reporter_);
    auto size_opt = const_evaluator.evaluate(node->size.get());

    if (!size_opt) {
        error_reporter_.report_error("Array size must be a constant expression.");
        resolved_type_ = nullptr;
        return;
    }

    size_t size = *size_opt;
    resolved_type_ = std::make_shared<ArrayType>(element_type, size);
}

void TypeResolver::visit(UnitTypeNode *node) { resolved_type_ = std::make_shared<UnitType>(); }

void TypeResolver::visit(TupleTypeNode *node) {
    // For now, just create a unit type - full tuple support later
    resolved_type_ = std::make_shared<UnitType>();
}

void TypeResolver::visit(PathTypeNode *node) {

    if (auto *var_expr = dynamic_cast<VariableExpr *>(node->path.get())) {
        const auto &name = var_expr->name.lexeme;

        if (name == "i32" || name == "isize") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::INTEGER);
        } else if (name == "u32" || name == "usize") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::UNSIGNED_INTEGER);
        } else if (name == "bool") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        } else if (name == "char") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::CHAR);
        } else if (name == "string") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::STRING);
        } else {
            auto symbol = symbol_table_.lookup(name);
            if (symbol && symbol->kind == Symbol::TYPE) {
                resolved_type_ = symbol->type;
                node->resolved_symbol = symbol;
            } else {
                resolved_type_ = nullptr;
            }
        }
    } else if (auto *path_expr = dynamic_cast<PathExpr *>(node->path.get())) {

        auto symbol = path_expr->accept(&name_resolver_);
        if (symbol && symbol->kind == Symbol::TYPE) {
            resolved_type_ = symbol->type;
            node->resolved_symbol = symbol;
        } else {
            resolved_type_ = nullptr;
        }
    } else {
        resolved_type_ = nullptr;
    }
}

void TypeResolver::visit(RawPointerTypeNode *node) {
    // Placeholder for pointer types
    resolved_type_ = nullptr;
}

void TypeResolver::visit(ReferenceTypeNode *node) {
    // For references, resolve the referenced type
    resolved_type_ = resolve(node->referenced_type.get());
}

void TypeResolver::visit(SliceTypeNode *node) { resolved_type_ = nullptr; }

void TypeResolver::visit(SelfTypeNode *node) { resolved_type_ = nullptr; }

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