#include "semantic.h"

// TypeCheckVisitor implementation

TypeCheckVisitor::TypeCheckVisitor(SymbolTable &symbol_table, ErrorReporter &error_reporter)
    : symbol_table_(symbol_table), error_reporter_(error_reporter) {}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(LiteralExpr *node) {
    const TokenType token_type = node->literal.type;
    switch (token_type) {
    case TokenType::NUMBER: {
        Number num = number_of_tokens(node->literal.lexeme, error_reporter_);
        if (num.Type == "Unknown") {
            error_reporter_.report_error("Invalid number format.", node->literal.line);
            return nullptr;
        }
        if (num.Type == "i32") {
            node->type = std::make_shared<PrimitiveType>(TypeKind::I32);
        } else if (num.Type == "u32") {
            node->type = std::make_shared<PrimitiveType>(TypeKind::U32);
        } else if (num.Type == "isize") {
            node->type = std::make_shared<PrimitiveType>(TypeKind::ISIZE);
        } else if (num.Type == "usize") {
            node->type = std::make_shared<PrimitiveType>(TypeKind::USIZE);
        } else if (num.Type == "anyint") {
            node->type = std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER);
        }
        break;
    }
    case TokenType::TRUE:
    case TokenType::FALSE:
        node->type = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        break;

    case TokenType::STRING: {
        auto str_primitive_type = std::make_shared<PrimitiveType>(TypeKind::STR);
        node->type = std::make_shared<ReferenceType>(str_primitive_type, false);
        break;
    }

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
    if (node->elements.empty()) {
        error_reporter_.report_error("Cannot infer type of empty array literal.");
        return nullptr;
    }

    for (auto &element : node->elements) {
        element->accept(this);
    }

    std::shared_ptr<Type> array_element_type = node->elements[0]->type;

    // Find a basic element type.
    if (array_element_type->kind == TypeKind::ANY_INTEGER) {
        for (const auto &element : node->elements) {
            if (is_concrete_integer(element->type->kind)) {
                array_element_type = element->type;
                break;
            }
        }
    }

    // Check all elements are of the same type.
    for (const auto &element : node->elements) {
        if (!element->type || !array_element_type->equals(element->type.get())) {
            error_reporter_.report_error(
                "Mismatched types in array literal. Expected element of type '" +
                array_element_type->to_string() + "' but found '" +
                (element->type ? element->type->to_string() : "unknown") + "'.");
            return nullptr;
        }
    }

    size_t array_size = node->elements.size();

    node->type = std::make_shared<ArrayType>(array_element_type, array_size);

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(ArrayInitializerExpr *node) {
    node->value->accept(this);
    std::shared_ptr<Type> element_type = node->value->type;

    if (!element_type) {
        return nullptr;
    }

    ConstEvaluator const_evaluator(symbol_table_, error_reporter_);

    std::optional<long long> size_opt = const_evaluator.evaluate(node->size.get());

    if (!size_opt) {
        error_reporter_.report_error("Array size must be a compile-time constant expression.");
        return nullptr;
    }

    long long evaluated_size = *size_opt;
    if (evaluated_size < 0) {
        error_reporter_.report_error("Array size cannot be negative.");
        return nullptr;
    }

    size_t array_size = static_cast<size_t>(evaluated_size);

    node->type = std::make_shared<ArrayType>(element_type, array_size);

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(VariableExpr *node) {
    if (node->resolved_symbol && node->resolved_symbol->type) {
        node->type = node->resolved_symbol->type;
    }
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnaryExpr *node) {
    node->right->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);

    auto left_type = node->left->type;
    auto right_type = node->right->type;

    if (!left_type || !right_type) {
        return nullptr;
    }

    switch (node->op.type) {
    case TokenType::PLUS:
    case TokenType::MINUS:
    case TokenType::STAR:
    case TokenType::SLASH:
    case TokenType::PERCENT: {

        bool left_is_int = is_any_integer_type(left_type->kind);
        bool right_is_int = is_any_integer_type(right_type->kind);

        if (left_is_int && right_is_int) {
            if (is_concrete_integer(left_type->kind)) {
                node->type = left_type;
            } else if (is_concrete_integer(right_type->kind)) {
                node->type = right_type;
            } else {
                node->type = std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER);
            }
        } else {
            error_reporter_.report_error("Arithmetic operations can only be performed on integers.",
                                         node->op.line);
        }
        break;
    }

    case TokenType::EQUAL_EQUAL:
    case TokenType::BANG_EQUAL: {
        bool is_valid = false;

        if (is_any_integer_type(left_type->kind) && is_any_integer_type(right_type->kind)) {
            is_valid = true;
        }

        else if (left_type->kind == TypeKind::BOOL && right_type->kind == TypeKind::BOOL) {
            is_valid = true;
        }

        else if (left_type->kind == TypeKind::REFERENCE &&
                 right_type->kind == TypeKind::REFERENCE) {

            if (left_type->equals(right_type.get())) {
                is_valid = true;
            }
        }

        if (is_valid) {
            node->type = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        } else {
            error_reporter_.report_error(
                "Invalid operands for equality operator. Operands must be of the same compatible "
                "type (integers, booleans, or references).",
                node->op.line);
        }
        break;
    }

    case TokenType::LESS:
    case TokenType::LESS_EQUAL:
    case TokenType::GREATER:
    case TokenType::GREATER_EQUAL: {

        bool left_is_int = is_any_integer_type(left_type->kind);
        bool right_is_int = is_any_integer_type(right_type->kind);

        if (left_is_int && right_is_int) {
            node->type = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        } else {
            error_reporter_.report_error(
                "Comparison operations are only supported for integers for now.", node->op.line);
        }
        break;
    }

    case TokenType::AMPERSAND_AMPERSAND:
    case TokenType::PIPE_PIPE: {
        if (left_type->kind == TypeKind::BOOL && right_type->kind == TypeKind::BOOL) {
            node->type = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        } else {
            error_reporter_.report_error("Logical operations can only be performed on booleans.",
                                         node->op.line);
        }
        break;
    }

    default:
        error_reporter_.report_error("Unsupported binary operator.", node->op.line);
        break;
    }

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(CallExpr *node) {

    node->callee->accept(this);
    for (auto &arg : node->arguments) {
        arg->accept(this);
    }

    if (node->callee->type->kind != TypeKind::FUNCTION) {
        error_reporter_.report_error("This expression is not callable.");
        return nullptr;
    }

    auto fn_type = std::dynamic_pointer_cast<FunctionType>(node->callee->type);

    if (node->arguments.size() != fn_type->param_types.size()) {
        error_reporter_.report_error("Expected " + std::to_string(fn_type->param_types.size()) +
                                     " arguments, but found " +
                                     std::to_string(node->arguments.size()) + ".");
        return nullptr;
    }

    for (size_t i = 0; i < node->arguments.size(); ++i) {
        auto &arg_type = node->arguments[i]->type;
        auto &param_type = fn_type->param_types[i];
        if (arg_type && !arg_type->equals(param_type.get())) {
            error_reporter_.report_error("Mismatched types. Expected argument type '" +
                                         param_type->to_string() + "' but found '" +
                                         arg_type->to_string() + "'.");
        }
    }

    node->type = fn_type->return_type;

    if (node->callee->resolved_symbol) {
        auto callee_symbol = node->callee->resolved_symbol;
        if (callee_symbol->name == "exit" && callee_symbol->is_builtin) {
            if (current_function_symbol_ && current_function_symbol_->name != "main") {
                error_reporter_.report_error(
                    "'exit' can only be called within the 'main' function.");
            }
        }
    }

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(IfExpr *node) {
    node->condition->accept(this);
    if (node->condition == nullptr || node->condition->type == nullptr ||
        node->condition->type->kind != TypeKind::BOOL) {
        error_reporter_.report_error("Condition expression of if must be of type 'bool'.");
        return nullptr;
    }
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(LoopExpr *node) {
    loop_depth_++;
    breakable_expr_type_stack_.push_back(nullptr);
    node->body->accept(this);
    loop_depth_--;
    if (breakable_expr_type_stack_.back() != nullptr) {
        node->type = breakable_expr_type_stack_.back();
    } else {
        node->type = std::make_shared<UnitType>();
    }
    breakable_expr_type_stack_.pop_back();
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(WhileExpr *node) {

    node->condition->accept(this);
    if (node->condition == nullptr || node->condition->type == nullptr ||
        node->condition->type->kind != TypeKind::BOOL) {
        error_reporter_.report_error("Condition expression of while must be of type 'bool'.");
        return nullptr;
    }
    loop_depth_++;

    breakable_expr_type_stack_.push_back(nullptr);
    node->body->accept(this);

    loop_depth_--;

    if (breakable_expr_type_stack_.back() != nullptr) {
        node->type = breakable_expr_type_stack_.back();
    } else {
        node->type = std::make_shared<UnitType>();
    }
    breakable_expr_type_stack_.pop_back();
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(IndexExpr *node) {
    node->object->accept(this);
    node->index->accept(this);

    auto object_type = node->object->type;
    auto index_type = node->index->type;

    if (!object_type || !index_type) {
        return nullptr;
    }

    if (object_type->kind != TypeKind::ARRAY) {
        error_reporter_.report_error("Type '" + object_type->to_string() + "' cannot be indexed.");
        return nullptr;
    }
    auto array_type = std::dynamic_pointer_cast<ArrayType>(object_type);

    if (!is_any_integer_type(index_type->kind)) {
        error_reporter_.report_error("Array index must be an integer.");
        return nullptr;
    }

    node->type = array_type->element_type;
    node->resolved_symbol = node->object->resolved_symbol;

    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    std::shared_ptr<Symbol> target_symbol = node->target->resolved_symbol;

    if (target_symbol) {
        if (!target_symbol->is_mutable) {
            error_reporter_.report_error("Cannot assign to immutable variable '" +
                                         target_symbol->name + "'.");
        }
    } else {
        error_reporter_.report_error("Undefined variable in assignment.");
    }

    if (node->target->type && node->value->type) {
        if (node->value->type->kind == TypeKind::ANY_INTEGER &&
            (node->target->type->kind == TypeKind::I32 ||
             node->target->type->kind == TypeKind::U32 ||
             node->target->type->kind == TypeKind::ISIZE ||
             node->target->type->kind == TypeKind::USIZE)) {
            // Nothing happens.
        } else if (!node->target->type->equals(node->value->type.get())) {
            error_reporter_.report_error(
                "Type mismatch in assignment. Cannot assign value of type '" +
                node->value->type->to_string() + "' to variable of type '" +
                node->target->type->to_string() + "'.");
        }
    }
    node->type = std::make_shared<UnitType>();
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(CompoundAssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(ReferenceExpr *node) {
    node->expression->accept(this);
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnderscoreExpr *node) { return nullptr; }

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

            if (!declared_type->equals(initializer_type.get())) {
                error_reporter_.report_error("Mismatched types. Expected '" +
                                             declared_type->to_string() + "' but found '" +
                                             initializer_type->to_string() + "'.");
            } else
                id_pattern->resolved_symbol->type = declared_type;
        }
    }
}

void TypeCheckVisitor::visit(ReturnStmt *node) {
    if (node->value) {
        (*node->value)->accept(this);
        auto actual_return_type = (*node->value)->type;

        if (actual_return_type && !actual_return_type->equals(current_return_type_.get())) {
            error_reporter_.report_error("Mismatched return type. Expected '" +
                                             current_return_type_->to_string() + "' but found '" +
                                             actual_return_type->to_string() + "'.",
                                         node->keyword.line);
        }
    } else {
        auto implicit_unit_type = std::make_shared<UnitType>();
        if (!implicit_unit_type->equals(current_return_type_.get())) {
            error_reporter_.report_error("This function should return a value of type '" +
                                             current_return_type_->to_string() +
                                             "', but the return statement is empty.",
                                         node->keyword.line);
        }
    }
}

void TypeCheckVisitor::visit(BreakStmt *node) {
    if (loop_depth_ == 0) {
        error_reporter_.report_error("'break' can only be used inside a loop.");
        return;
    }
    if (breakable_expr_type_stack_.empty()) {
        error_reporter_.report_error("Internal error: breakable expression type stack is empty.");
        return;
    }
    auto &expected_type = breakable_expr_type_stack_.back();
    if (node->value) {
        (*node->value)->accept(this);
        if (expected_type == nullptr) {
            expected_type = (*node->value)->type;
        } else if ((*node->value)->type && !expected_type->equals((*node->value)->type.get())) {
            error_reporter_.report_error("Mismatched types in 'break' expression. Expected type '" +
                                         expected_type->to_string() + "' but found '" +
                                         (*node->value)->type->to_string() + "'.");
        }
    } else {
        if (expected_type == nullptr) {
            expected_type = std::make_shared<UnitType>();
        } else if (!expected_type->equals(std::make_shared<UnitType>().get())) {
            error_reporter_.report_error("Mismatched types in 'break' expression. Expected type '" +
                                         expected_type->to_string() + "' but found '()'.");
        }
    }
}

void TypeCheckVisitor::visit(ContinueStmt *node) {
    if (loop_depth_ == 0) {
        error_reporter_.report_error("'continue' can only be used inside a loop.");
        return;
    }
}

void TypeCheckVisitor::visit(TypeNameNode *node) {}

void TypeCheckVisitor::visit(ArrayTypeNode *node) {
    node->element_type->accept(this);
    node->size->accept(this);
}

void TypeCheckVisitor::visit(UnitTypeNode *node) {}

void TypeCheckVisitor::visit(TupleTypeNode *node) {
    for (auto &element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(FnDecl *node) {
    Symbol *previous_function = current_function_symbol_;
    current_function_symbol_ = node->resolved_symbol.get();

    std::shared_ptr<Type> previous_return_type = current_return_type_;
    if (node->resolved_symbol && node->resolved_symbol->type->kind == TypeKind::FUNCTION) {
        auto fn_type = std::dynamic_pointer_cast<FunctionType>(node->resolved_symbol->type);
        current_return_type_ = fn_type->return_type;
    } else {
        current_return_type_ = std::make_shared<UnitType>();
    }

    if (node->body) {
        (*node->body)->accept(this);
    }

    if (current_function_symbol_ && current_function_symbol_->name == "main") {
        if (node->body) {
            check_main_for_early_exit((*node->body).get());
        }
    }

    current_function_symbol_ = previous_function;
    current_return_type_ = previous_return_type;
}

// Pattern visitors for TypeCheckVisitor
void TypeCheckVisitor::visit(IdentifierPattern *node) {
    // Pattern type checking would go here
}

void TypeCheckVisitor::visit(ReferencePattern *node) {}

void TypeCheckVisitor::visit(WildcardPattern *node) {}

void TypeCheckVisitor::visit(LiteralPattern *node) {}

void TypeCheckVisitor::visit(TuplePattern *node) {}

void TypeCheckVisitor::visit(SlicePattern *node) {}

void TypeCheckVisitor::visit(StructPattern *node) {}

void TypeCheckVisitor::visit(RestPattern *node) {}

// Missing expression visitors for TypeCheckVisitor
std::shared_ptr<Symbol> TypeCheckVisitor::visit(StructInitializerExpr *node) {
    node->name->accept(this);
    for (auto &field : node->fields) {
        field->value->accept(this);
    }
    return nullptr; // TODO: Implement proper type checking
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(UnitExpr *node) {
    node->type = std::make_shared<UnitType>();
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(GroupingExpr *node) {
    node->expression->accept(this);
    node->type = node->expression->type;
    return nullptr;
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
    if (node->target_type->resolved_symbol && node->target_type->resolved_symbol->type) {
        if (node->expression->type->kind != TypeKind::ANY_INTEGER &&
            node->expression->type->kind != TypeKind::CHAR &&
            node->expression->type->kind != TypeKind::BOOL &&
            !is_concrete_integer(node->expression->type->kind)) {
            error_reporter_.report_error("The expression's type is not supported: " +
                                         node->expression->type->to_string() + ".");
            return nullptr;
        }
        if (!is_concrete_integer(node->target_type->resolved_symbol->type->kind)) {
            error_reporter_.report_error("The target type is not supported: " +
                                         node->target_type->resolved_symbol->type->to_string() +
                                         ".");
            return nullptr;
        }
        node->type = node->target_type->resolved_symbol->type;
    }
    return nullptr;
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
    // for (auto &variant : node->variants) {
    //  EnumVariant doesn't have accept method, handle manually if needed
    //  variant->name is just an identifier
    //  variant->fields would need type checking if present
    // }
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

void TypeCheckVisitor::check_main_for_early_exit(BlockStmt *body) {
    if (!body || body->statements.empty()) {
        return;
    }

    size_t statements_to_check = body->statements.size() - 1;

    for (size_t i = 0; i < statements_to_check; ++i) {
        auto &stmt = body->statements[i];

        auto *expr_stmt = dynamic_cast<ExprStmt *>(stmt.get());
        if (!expr_stmt) {
            continue;
        }

        auto *call_expr = dynamic_cast<CallExpr *>(expr_stmt->expression.get());
        if (!call_expr) {
            continue;
        }

        if (call_expr->callee->resolved_symbol) {
            const auto &callee_symbol = call_expr->callee->resolved_symbol;

            if (callee_symbol->name == "exit" && callee_symbol->is_builtin) {
                error_reporter_.report_error(
                    "Built-in function 'exit' must be the final statement in 'main'.");
            }
        }
    }
}