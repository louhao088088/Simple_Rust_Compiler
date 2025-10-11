// name_resolution.cpp

#include "semantic.h"

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

    auto symbol = symbol_table_.lookup_value(node->name.lexeme);

    if (!symbol) {
        symbol = symbol_table_.lookup_type(node->name.lexeme);
    }

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

    current_type_ = var_type;
    node->pattern->accept(this);
    current_type_ = nullptr;
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
    if (node->size) {
        node->size->accept(this);
    }
}

void NameResolutionVisitor::visit(UnitTypeNode *node) {}

void NameResolutionVisitor::visit(TupleTypeNode *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(FnDecl *node) {
    std::vector<std::shared_ptr<Type>> param_types;
    for (const auto &param : node->params) {
        if (param->type) {
            auto param_type = type_resolver_.resolve(param->type.get());
            if (param_type) {
                param_types.push_back(param_type);
            } else {
                error_reporter_.report_error("Could not resolve type for parameter.");
            }
        } else {
            error_reporter_.report_error("Function parameters must have a type annotation.");
        }
    }

    std::shared_ptr<Type> return_type;
    if (node->return_type) {
        return_type = type_resolver_.resolve((*node->return_type).get());
    } else {
        return_type = std::make_shared<UnitType>();
    }

    auto function_type = std::make_shared<FunctionType>(return_type, param_types);

    auto fn_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::FUNCTION, function_type);
    if (!symbol_table_.define_value(node->name.lexeme, fn_symbol)) {
        error_reporter_.report_error("Function '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
    }
    node->resolved_symbol = fn_symbol;

    if (node->body) {
        symbol_table_.enter_scope();

        for (size_t i = 0; i < node->params.size(); ++i) {
            const auto &param = node->params[i];
            std::shared_ptr<Type> type_for_this_param = param_types[i];
            current_type_ = type_for_this_param;
            if (auto ident = dynamic_cast<IdentifierPattern *>(param->pattern.get())) {
                auto param_symbol =
                    std::make_shared<Symbol>(ident->name.lexeme, Symbol::VARIABLE, current_type_);
                param_symbol->is_mutable = ident->is_mutable;
                if (!symbol_table_.define_value(ident->name.lexeme, param_symbol)) {
                    error_reporter_.report_error("Parameter '" + ident->name.lexeme +
                                                     "' is already defined.",
                                                 ident->name.line);
                }
                ident->resolved_symbol = param_symbol;
            } else {
                param->pattern->accept(this);
            }
            current_type_ = nullptr;
        }

        (*node->body)->accept(this);

        symbol_table_.exit_scope();
    }
}

// Pattern visitors
void NameResolutionVisitor::visit(IdentifierPattern *node) {
    auto var_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::VARIABLE, current_type_);
    var_symbol->is_mutable = node->is_mutable;
    symbol_table_.define_variable(node->name.lexeme, var_symbol, true);
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

void NameResolutionVisitor::visit(ReferencePattern *node) {
    auto *current_ref_type = dynamic_cast<ReferenceType *>(current_type_.get());
    if (!current_ref_type) {
        error_reporter_.report_error(
            "Pattern mismatch: expected a reference type, but the value is not a reference.");
        return;
    }

    if (node->is_mutable && !current_ref_type->is_mutable) {
        error_reporter_.report_error("Cannot bind immutable reference to a mutable pattern.");
    }

    auto inner_type = current_ref_type->referenced_type;

    auto original_type = current_type_;
    current_type_ = inner_type;

    node->pattern->accept(this);

    current_type_ = original_type;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(StructInitializerExpr *node) {

    auto type_name_opt = get_name_from_expr(node->name.get());
    if (!type_name_opt) {
        error_reporter_.report_error("Struct initializer failed by name.");
        return nullptr;
    }
    std::string type_name = *type_name_opt;

    auto struct_symbol = symbol_table_.lookup_type(type_name);

    if (!struct_symbol) {
        error_reporter_.report_error("Unknown type '" + type_name + "'.");
        return nullptr;
    }
    if (struct_symbol->kind != Symbol::TYPE || !struct_symbol->type ||
        struct_symbol->type->kind != TypeKind::STRUCT) {
        error_reporter_.report_error("'" + type_name + "' is not a struct type.");
        return nullptr;
    }

    node->resolved_symbol = struct_symbol;

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
    std::shared_ptr<Type> var_type = nullptr;

    node->target_type->accept(this);
    if (node->target_type) {
        var_type = type_resolver_.resolve(node->target_type.get());
        if (!var_type) {
            error_reporter_.report_error("Cannot resolve As expression for variable.");
        }
    } else {
        error_reporter_.report_error("As expression must have a type annotation.");
    }
    return nullptr;
}

std::shared_ptr<Symbol> NameResolutionVisitor::visit(MatchExpr *node) {
    node->scrutinee->accept(this);
    for (auto &arm : node->arms) {
        arm->pattern->accept(this);
        if (arm->guard)
            (*arm->guard)->accept(this);
        arm->body->accept(this);
    }
    return nullptr;
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

    auto right_name_opt = get_name_from_expr(node->right.get());
    if (!right_name_opt) {
        error_reporter_.report_error("Invalid right-hand side of a '::' path");
        return nullptr;
    }
    std::string right_name = *right_name_opt;

    if (!left_symbol->members) {
        error_reporter_.report_error("'" + left_symbol->name + "' has no members to look up.");
        return nullptr;
    }

    auto final_symbol = left_symbol->members->lookup_value(right_name);

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
    if (symbol_table_.lookup_type(node->name.lexeme)) {
        error_reporter_.report_error("Type '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
        return;
    }
    auto struct_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::TYPE);
    auto struct_type =
        std::make_shared<StructType>(node->name.lexeme, std::weak_ptr<Symbol>(struct_symbol));
    struct_symbol->type = struct_type;

    symbol_table_.define_type(node->name.lexeme, struct_symbol);
    node->resolved_symbol = struct_symbol;

    for (const auto &field_node : node->fields) {
        auto field_type = type_resolver_.resolve(field_node->type.get());
        if (!field_type) {
            error_reporter_.report_error("Unknown field type.", field_node->name.line);
            continue;
        }
        struct_type->fields[field_node->name.lexeme] = field_type;
        auto field_symbol =
            std::make_shared<Symbol>(field_node->name.lexeme, Symbol::VARIABLE, field_type);
        struct_symbol->members->define_value(field_node->name.lexeme, field_symbol);
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

    if (!symbol_table_.define_value(node->name.lexeme, const_symbol)) {
        error_reporter_.report_error("Constant '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
    }
    node->resolved_symbol = const_symbol;

    node->value->accept(this);
}

void NameResolutionVisitor::visit(EnumDecl *node) {

    auto enum_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::TYPE);
    auto enum_type =
        std::make_shared<EnumType>(node->name.lexeme, std::weak_ptr<Symbol>(enum_symbol));
    enum_symbol->type = enum_type;
    if (symbol_table_.lookup_type(node->name.lexeme)) {
        error_reporter_.report_error("Type '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
        return;
    }

    symbol_table_.define_type(node->name.lexeme, enum_symbol);
    node->resolved_symbol = enum_symbol;

    for (const auto &variant : node->variants) {
        auto variant_symbol = std::make_shared<Symbol>(variant->name.lexeme, Symbol::VARIANT);
        if (!enum_symbol->members->define_value(variant->name.lexeme, variant_symbol)) {
            error_reporter_.report_error("Enum variant '" + variant->name.lexeme +
                                             "' is already defined.",
                                         variant->name.line);
        }
    }
}

void NameResolutionVisitor::visit(ModDecl *node) {}

void NameResolutionVisitor::visit(TraitDecl *node) {}

void NameResolutionVisitor::visit(ImplBlock *node) {
    if (node->trait_name)
        (*node->trait_name)->accept(this);
    node->target_type->accept(this);

    symbol_table_.enter_scope();

    auto target_type_symbol = node->target_type->resolved_symbol;
    if (target_type_symbol) {
        auto self_symbol = std::make_shared<Symbol>("Self", Symbol::TYPE);
        self_symbol->aliased_symbol = target_type_symbol;
        symbol_table_.define_type("Self", self_symbol);
    }

    for (auto &item : node->implemented_items) {
        item->accept(this);
    }

    symbol_table_.exit_scope();
}

void NameResolutionVisitor::declare_struct(StructDecl *node) {
    if (symbol_table_.lookup_type(node->name.lexeme)) {
        error_reporter_.report_error("Type '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
        return;
    }

    auto struct_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::TYPE);
    auto struct_type =
        std::make_shared<StructType>(node->name.lexeme, std::weak_ptr<Symbol>(struct_symbol));
    struct_symbol->type = struct_type;

    symbol_table_.define_type(node->name.lexeme, struct_symbol);
    node->resolved_symbol = struct_symbol;
}

void NameResolutionVisitor::declare_function(FnDecl *node) {
    std::vector<std::shared_ptr<Type>> param_types;
    for (const auto &param : node->params) {
        if (param->type) {
            auto param_type = type_resolver_.resolve(param->type.get());
            if (param_type) {
                param_types.push_back(param_type);
            } else {
                error_reporter_.report_error("Could not resolve type for parameter.");
            }
        } else {
            error_reporter_.report_error("Function parameters must have a type annotation.");
        }
    }

    std::shared_ptr<Type> return_type;
    if (node->return_type) {
        return_type = type_resolver_.resolve((*node->return_type).get());
    } else {
        return_type = std::make_shared<UnitType>();
    }

    auto function_type = std::make_shared<FunctionType>(return_type, param_types);

    auto fn_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::FUNCTION, function_type);
    if (!symbol_table_.define_value(node->name.lexeme, fn_symbol)) {
        error_reporter_.report_error("Function '" + node->name.lexeme + "' is already defined.",
                                     node->name.line);
    }
    node->resolved_symbol = fn_symbol;
    node->resolved_symbol->is_main = (node->name.lexeme == "main");
}

void NameResolutionVisitor::declare_impl_method(ImplBlock *node) {
    auto target_type = type_resolver_.resolve(node->target_type.get());
    if (!target_type || target_type->kind != TypeKind::STRUCT) {
        error_reporter_.report_error("Impl block target type must be a struct.");
        return;
    }
    auto target_type_symbol = node->target_type->resolved_symbol;

    auto struct_type = std::static_pointer_cast<StructType>(target_type);
    auto struct_symbol = struct_type->symbol.lock();
    if (!struct_symbol) {
        error_reporter_.report_error("Could not resolve struct type for impl block.");
        return;
    }

    symbol_table_.enter_scope();
    symbol_table_.define_type("Self", struct_symbol);

    for (auto &item : node->implemented_items) {
        if (auto *fn_decl = dynamic_cast<FnDecl *>(item.get())) {
            std::vector<std::shared_ptr<Type>> param_types;
            for (const auto &param : fn_decl->params) {
                auto param_type = type_resolver_.resolve(param->type.get());
                if (param_type)
                    param_types.push_back(param_type);
            }
            auto return_type = fn_decl->return_type
                                   ? type_resolver_.resolve((*fn_decl->return_type).get())
                                   : std::make_shared<UnitType>();

            auto method_type = std::make_shared<FunctionType>(return_type, param_types);
            auto method_symbol =
                std::make_shared<Symbol>(fn_decl->name.lexeme, Symbol::FUNCTION, method_type);
            fn_decl->resolved_symbol = method_symbol;
            bool is_instance_method = false;
            if (!fn_decl->params.empty()) {
                if (auto ident_pattern =
                        dynamic_cast<IdentifierPattern *>(fn_decl->params[0]->pattern.get())) {
                    if (ident_pattern->name.lexeme == "self") {
                        is_instance_method = true;
                    }
                }
            }

            if (is_instance_method) {
                if (!struct_type->members->define_value(fn_decl->name.lexeme, method_symbol)) {
                    error_reporter_.report_error("Method '" + fn_decl->name.lexeme +
                                                     "' already defined for this struct.",
                                                 fn_decl->name.line);
                }
            } else {
                if (!target_type_symbol->members->define_value(fn_decl->name.lexeme,
                                                               method_symbol)) {
                    error_reporter_.report_error("Function '" + fn_decl->name.lexeme +
                                                     "' already defined for this type.",
                                                 fn_decl->name.line);
                }
            }
        }
    }
    symbol_table_.exit_scope();
}

void NameResolutionVisitor::define_function_body(FnDecl *node) {
    std::vector<std::shared_ptr<Type>> param_types;
    for (const auto &param : node->params) {
        if (param->type) {
            auto param_type = type_resolver_.resolve(param->type.get());
            if (param_type) {
                param_types.push_back(param_type);
            } else {
                error_reporter_.report_error("Could not resolve type for parameter.");
            }
        } else {
            error_reporter_.report_error("Function parameters must have a type annotation.");
        }
    }
    if (node->body) {
        symbol_table_.enter_scope();

        std::vector<std::shared_ptr<Item>> inner_items;
        for (auto &stmt : (*node->body)->statements) {
            if (auto item_stmt = dynamic_cast<ItemStmt *>(stmt.get())) {
                inner_items.push_back(item_stmt->item);
            }
        }

        for (const auto &item : inner_items) {
            if (auto decl = dynamic_cast<StructDecl *>(item.get()))
                declare_struct(decl);
            else if (auto decl = dynamic_cast<EnumDecl *>(item.get()))
                item->accept(this);
            else if (auto decl = dynamic_cast<ConstDecl *>(item.get()))
                item->accept(this);
        }

        for (const auto &item : inner_items) {
            if (auto decl = dynamic_cast<FnDecl *>(item.get()))
                declare_function(decl);
        }

        for (const auto &item : inner_items) {
            if (auto decl = dynamic_cast<ImplBlock *>(item.get())) {
                declare_impl_method(decl);
            } else if (auto decl = dynamic_cast<StructDecl *>(item.get())) {
                define_struct_body(decl);
            }
        }

        for (size_t i = 0; i < node->params.size(); ++i) {
            const auto &param = node->params[i];

            std::shared_ptr<Type> type_for_this_param = param_types[i];

            current_type_ = type_for_this_param;
            param->pattern->accept(this);
            current_type_ = nullptr;
        }

        for (auto &stmt : (*node->body)->statements) {
            if (auto item_stmt = dynamic_cast<ItemStmt *>(stmt.get())) {
                if (auto *decl = dynamic_cast<FnDecl *>(item_stmt->item.get())) {
                    define_function_body(decl);
                } else if (auto *decl = dynamic_cast<ImplBlock *>(item_stmt->item.get())) {
                    for (auto &item : decl->implemented_items) {
                        auto target_type = type_resolver_.resolve(decl->target_type.get());
                        auto target_type_symbol =
                            std::dynamic_pointer_cast<StructType>(target_type)->symbol.lock();

                        symbol_table_.enter_scope();
                        symbol_table_.define_type("Self", target_type_symbol);
                        if (auto *fn_decl = dynamic_cast<FnDecl *>(item.get())) {
                            define_function_body(fn_decl);
                        }
                        symbol_table_.exit_scope();
                    }
                }
                continue;
            }
            stmt->accept(this);
        }
        if ((*node->body)->final_expr) {
            (*(*node->body)->final_expr)->accept(this);
        }

        symbol_table_.exit_scope();
    }
}

void NameResolutionVisitor::define_struct_body(StructDecl *node) {

    auto struct_symbol = node->resolved_symbol;
    auto struct_type = std::static_pointer_cast<StructType>(struct_symbol->type);

    for (const auto &field_node : node->fields) {
        auto field_type = type_resolver_.resolve(field_node->type.get());
        if (!field_type) {
            error_reporter_.report_error(
                "Unknown type for field '" + field_node->name.lexeme + "'.", field_node->name.line);
            continue;
        }

        struct_type->fields[field_node->name.lexeme] = field_type;
        auto field_symbol =
            std::make_shared<Symbol>(field_node->name.lexeme, Symbol::VARIABLE, field_type);
        struct_type->members->define_value(field_node->name.lexeme, field_symbol);
    }
}

void NameResolutionVisitor::resolve(Program *ast) {
    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<ConstDecl *>(item.get())) {
            decl->accept(this);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<EnumDecl *>(item.get())) {
            decl->accept(this);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<StructDecl *>(item.get())) {
            declare_struct(decl);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<FnDecl *>(item.get())) {
            declare_function(decl);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<ImplBlock *>(item.get())) {
            declare_impl_method(decl);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<StructDecl *>(item.get())) {
            define_struct_body(decl);
        }
    }

    for (auto &item : ast->items) {
        if (auto *decl = dynamic_cast<FnDecl *>(item.get())) {
            define_function_body(decl);
        } else if (auto *decl = dynamic_cast<ImplBlock *>(item.get())) {
            for (auto &item : decl->implemented_items) {
                auto target_type = type_resolver_.resolve(decl->target_type.get());
                auto target_type_symbol =
                    std::dynamic_pointer_cast<StructType>(target_type)->symbol.lock();

                symbol_table_.enter_scope();
                symbol_table_.define_type("Self", target_type_symbol);
                if (auto *fn_decl = dynamic_cast<FnDecl *>(item.get())) {
                    define_function_body(fn_decl);
                }
                symbol_table_.exit_scope();
            }
        }
    }
}
