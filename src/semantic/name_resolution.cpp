#include "semantic.h"

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
    var_symbol->is_mutable = node->is_mutable;
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
