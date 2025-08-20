// semantic.cpp

#include "semantic.h"

Number number_of_tokens(string token) {
    long long num = -1;
    if (token.length() > 2 && token[0] == '0' && token[1] == 'x') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '9') || !(token[i] >= 'a' && token[i] <= 'f') ||
                !(token[i] >= 'A' && token[i] <= 'F')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            puts("Integer overflow");
                            return {-1, false};
                        }
                        return {num, false};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            puts("Unsigned integer overflow");
                            return {-1, false};
                        }
                        return {num, true};
                    }

                } else {
                    puts("Invalid number format");
                    return {-1, false};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 16 +
                  (token[i] >= '0' && token[i] <= '9'
                       ? token[i] - '0'
                       : (token[i] >= 'a' && token[i] <= 'f'
                              ? token[i] - 'a' + 10
                              : (token[i] >= 'A' && token[i] <= 'F' ? token[i] - 'A' + 10 : -1)));
        }
    } else if (token.length() > 2 && token[0] == '0' && token[1] == 'b') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (token[i] != '0' && token[i] != '1') {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            puts("Integer overflow");
                            return {-1, false};
                        }
                        return {num, false};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            puts("Unsigned integer overflow");
                            return {-1, false};
                        }
                        return {num, true};
                    }

                } else {
                    puts("Invalid number format");
                    return {-1, false};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 2 + (token[i] == '0' ? 0 : 1);
        }

    } else if (token.length() > 1 && token[0] == '0' && token[1] == 'o') {
        for (size_t i = 2; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '7')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            puts("Integer overflow");
                            return {-1, false};
                        }
                        return {num, false};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            puts("Unsigned integer overflow");
                            return {-1, false};
                        }
                        return {num, true};
                    }

                } else {
                    puts("Invalid number format");
                    return {-1, false};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 8 + (token[i] - '0');
        }

    } else {
        if (token[0] == '_') {
            puts("Invalid number format");
            return {-1, false};
        }
        for (size_t i = 0; i < token.length(); i++) {
            if (token[i] == '_')
                continue;

            if (!(token[i] >= '0' && token[i] <= '9')) {
                if ((token[i] == 'i' || token[i] == 'u') &&
                    ((token.length() == i + 2 && token[i + 1] == '3' && token[i + 2] == '2') ||
                     (token.length() == i + 4 && token[i + 1] == 's' && token[i + 2] == 'i' &&
                      token[i + 3] == 'z' && token[i + 4] == 'e'))) {
                    if (token[i] == 'i') {
                        if (num > 2147483647) {
                            puts("Integer overflow");
                            return {-1, false};
                        }
                        return {num, false};
                    } else if (token[i] == 'u') {
                        if (num > 4294967295) {
                            puts("Unsigned integer overflow");
                            return {-1, false};
                        }
                        return {num, true};
                    }

                } else {
                    puts("Invalid number format");
                    return {-1, false};
                }
            }
            if (num == -1)
                num = 0;
            num = num * 10 + (token[i] - '0');
        }
    }

    if (num < 0) {
        puts("Invalid number format");
        return {-1, false};
    }
    return {num, true};
}

// SymbolTable implementation
void SymbolTable::enter_scope() { scopes_.emplace_back(); }

void SymbolTable::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void SymbolTable::define(const std::string &name, std::shared_ptr<Symbol> symbol) {
    if (!scopes_.empty()) {
        scopes_.back()[name] = symbol;
    }
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
    : error_reporter_(error_reporter) {
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
    if (node->initializer) {
        (*node->initializer)->accept(this);
    }
    if (node->pattern) {
        node->pattern->accept(this);
    }
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
            param->accept(this);
        }
        (*node->body)->accept(this);
        symbol_table_.exit_scope();
    }
}

void NameResolutionVisitor::visit(Program *node) {
    for (auto &item : node->items) {
        item->accept(this);
    }
}

// Pattern visitors
void NameResolutionVisitor::visit(IdentifierPattern *node) {
    auto var_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::VARIABLE);
    symbol_table_.define(node->name.lexeme, var_symbol);
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
        arm->accept(this);
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
    // TODO: Type check path expressions
    return nullptr; // TODO: Implement proper type checking
}

// Missing statement visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(ItemStmt *node) { node->item->accept(this); }

// Missing type node visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(PathTypeNode *node) { node->path->accept(this); }

void NameResolutionVisitor::visit(RawPointerTypeNode *node) { node->pointee_type->accept(this); }

void NameResolutionVisitor::visit(ReferenceTypeNode *node) { node->referenced_type->accept(this); }

void NameResolutionVisitor::visit(SliceTypeNode *node) { node->element_type->accept(this); }

void NameResolutionVisitor::visit(SelfTypeNode *node) {}

// Missing item visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(StructDecl *node) {}

void NameResolutionVisitor::visit(ConstDecl *node) {
    node->value->accept(this);
    if (node->type)
        node->type->accept(this);
}

void NameResolutionVisitor::visit(EnumDecl *node) {
    for (auto &variant : node->variants) {
        variant->accept(this);
    }
}

void NameResolutionVisitor::visit(ModDecl *node) {
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
    for (auto &item : node->implemented_items) {
        item->accept(this);
    }
}

// Missing other visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(EnumVariant *node) {
    // TODO: Handle enum variant fields
}

void NameResolutionVisitor::visit(MatchArm *node) {
    node->pattern->accept(this);
    if (node->guard)
        (*node->guard)->accept(this);
    node->body->accept(this);
}

// TypeCheckVisitor implementation
TypeCheckVisitor::TypeCheckVisitor(ErrorReporter &error_reporter)
    : error_reporter_(error_reporter) {}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(LiteralExpr *node) {
    // TODO: Infer type from literal token
    return nullptr; // TODO: Implement proper type checking
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
    // TODO: Type check binary operations
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
    if (node->initializer) {
        (*node->initializer)->accept(this);
    }
    // TODO: Type check let statement
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

void TypeCheckVisitor::visit(Program *node) {
    for (auto &item : node->items) {
        item->accept(this);
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
        arm->accept(this);
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
        variant->accept(this);
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

// Missing other visitors for TypeCheckVisitor
void TypeCheckVisitor::visit(EnumVariant *node) {
    // TODO: Handle enum variant type checking
}

void TypeCheckVisitor::visit(MatchArm *node) {
    node->pattern->accept(this);
    if (node->guard)
        (*node->guard)->accept(this);
    node->body->accept(this);
}
