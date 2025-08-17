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

void NameResolutionVisitor::visit(LiteralExpr *node) {}

void NameResolutionVisitor::visit(ArrayLiteralExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(ArrayInitializerExpr *node) {
    node->value->accept(this);
    node->size->accept(this);
}

void NameResolutionVisitor::visit(VariableExpr *node) {
    auto symbol = symbol_table_.lookup(node->name.lexeme);
    if (!symbol) {
        error_reporter_.report_error("Undefined variable '" + node->name.lexeme + "'",
                                     node->name.line, node->name.column);
    } else {
        node->resolved_symbol = symbol;
    }
}

void NameResolutionVisitor::visit(UnaryExpr *node) { node->right->accept(this); }

void NameResolutionVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);
}

void NameResolutionVisitor::visit(CallExpr *node) {
    node->callee->accept(this);
    for (auto &arg : node->arguments) {
        arg->accept(this);
    }
}

void NameResolutionVisitor::visit(IfExpr *node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
}

void NameResolutionVisitor::visit(LoopExpr *node) { node->body->accept(this); }

void NameResolutionVisitor::visit(WhileExpr *node) {
    node->condition->accept(this);
    node->body->accept(this);
}

void NameResolutionVisitor::visit(IndexExpr *node) {
    node->object->accept(this);
    node->index->accept(this);
}

void NameResolutionVisitor::visit(FieldAccessExpr *node) { node->object->accept(this); }

void NameResolutionVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
}

void NameResolutionVisitor::visit(CompoundAssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
}

void NameResolutionVisitor::visit(ReferenceExpr *node) { node->expression->accept(this); }

void NameResolutionVisitor::visit(UnderscoreExpr *node) {}

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

// Pattern visitors (simplified implementations)
void NameResolutionVisitor::visit(IdentifierPattern *node) {
    // Define the variable symbol
    auto var_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::VARIABLE);
    symbol_table_.define(node->name.lexeme, var_symbol);
}

void NameResolutionVisitor::visit(WildcardPattern *node) {
    // Wildcard patterns don't bind variables
}

void NameResolutionVisitor::visit(LiteralPattern *node) {
    // Literal patterns don't bind variables
}

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

void NameResolutionVisitor::visit(StructPattern *node) {
    // TODO: Handle struct pattern fields
}

void NameResolutionVisitor::visit(RestPattern *node) {
    // Rest patterns don't bind variables themselves
}

// Missing expression visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(StructInitializerExpr *node) {
    node->name->accept(this);
    for (auto &field : node->fields) {
        field->value->accept(this);
    }
}

void NameResolutionVisitor::visit(UnitExpr *node) {
    // Unit expressions have no sub-expressions to visit
}

void NameResolutionVisitor::visit(GroupingExpr *node) { node->expression->accept(this); }

void NameResolutionVisitor::visit(TupleExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(AsExpr *node) {
    node->expression->accept(this);
    node->target_type->accept(this);
}

void NameResolutionVisitor::visit(MatchExpr *node) {
    node->scrutinee->accept(this);
    for (auto &arm : node->arms) {
        arm->accept(this);
    }
}

void NameResolutionVisitor::visit(PathExpr *node) {
    // TODO: Resolve path expressions
}

void NameResolutionVisitor::visit(RangeExpr *node) {
    if (node->start)
        (*node->start)->accept(this);
    if (node->end)
        (*node->end)->accept(this);
}

// Missing statement visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(ItemStmt *node) { node->item->accept(this); }

// Missing type node visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(PathTypeNode *node) { node->path->accept(this); }

void NameResolutionVisitor::visit(RawPointerTypeNode *node) { node->pointee_type->accept(this); }

void NameResolutionVisitor::visit(ReferenceTypeNode *node) { node->referenced_type->accept(this); }

void NameResolutionVisitor::visit(SliceTypeNode *node) { node->element_type->accept(this); }

void NameResolutionVisitor::visit(SelfTypeNode *node) {
    // Self types are handled in context
}

// Missing item visitors for NameResolutionVisitor
void NameResolutionVisitor::visit(StructDecl *node) {
    // TODO: Handle struct declarations
}

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

void TypeCheckVisitor::visit(LiteralExpr *node) {
    // TODO: Infer type from literal token
}

void TypeCheckVisitor::visit(ArrayLiteralExpr *node) {
    // TODO: Type check array elements and infer array type
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(ArrayInitializerExpr *node) {
    node->value->accept(this);
    node->size->accept(this);
    // TODO: Type check and infer array type
}

void TypeCheckVisitor::visit(VariableExpr *node) {
    if (node->resolved_symbol && node->resolved_symbol->type) {
        node->type = node->resolved_symbol->type;
    }
}

void TypeCheckVisitor::visit(UnaryExpr *node) {
    node->right->accept(this);
    // TODO: Type check unary operations
}

void TypeCheckVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);
    // TODO: Type check binary operations
}

void TypeCheckVisitor::visit(CallExpr *node) {
    node->callee->accept(this);
    for (auto &arg : node->arguments) {
        arg->accept(this);
    }
    // TODO: Type check function call
}

void TypeCheckVisitor::visit(IfExpr *node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
    // TODO: Type check if expression
}

void TypeCheckVisitor::visit(LoopExpr *node) {
    node->body->accept(this);
    // TODO: Loop expressions return ()
}

void TypeCheckVisitor::visit(WhileExpr *node) {
    node->condition->accept(this);
    node->body->accept(this);
    // TODO: While expressions return ()
}

void TypeCheckVisitor::visit(IndexExpr *node) {
    node->object->accept(this);
    node->index->accept(this);
    // TODO: Type check array/index access
}

void TypeCheckVisitor::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    // TODO: Type check field access
}

void TypeCheckVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    // TODO: Type check assignment compatibility
}

void TypeCheckVisitor::visit(CompoundAssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    // TODO: Type check compound assignment compatibility
}

void TypeCheckVisitor::visit(ReferenceExpr *node) {
    node->expression->accept(this);
    // TODO: Handle reference type checking
}

void TypeCheckVisitor::visit(UnderscoreExpr *node) {
    // Underscore expression has unknown type for now
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
void TypeCheckVisitor::visit(StructInitializerExpr *node) {
    node->name->accept(this);
    for (auto &field : node->fields) {
        field->value->accept(this);
    }
}

void TypeCheckVisitor::visit(UnitExpr *node) {
    // Unit expressions have unit type
}

void TypeCheckVisitor::visit(GroupingExpr *node) { node->expression->accept(this); }

void TypeCheckVisitor::visit(TupleExpr *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(AsExpr *node) {
    node->expression->accept(this);
    node->target_type->accept(this);
}

void TypeCheckVisitor::visit(MatchExpr *node) {
    node->scrutinee->accept(this);
    for (auto &arm : node->arms) {
        arm->accept(this);
    }
}

void TypeCheckVisitor::visit(PathExpr *node) {
    // TODO: Type check path expressions
}

void TypeCheckVisitor::visit(RangeExpr *node) {
    if (node->start)
        (*node->start)->accept(this);
    if (node->end)
        (*node->end)->accept(this);
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
