#include "semantic.h"

// TypeCheckVisitor implementation

TypeCheckVisitor::TypeCheckVisitor(SymbolTable &symbol_table, BuiltinTypes &builtin_types,
                                   ErrorReporter &error_reporter)
    : symbol_table_(symbol_table), builtin_types_(builtin_types), error_reporter_(error_reporter) {}

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
            node->type = builtin_types_.i32_type;
        } else if (num.Type == "u32") {
            node->type = builtin_types_.u32_type;
        } else if (num.Type == "isize") {
            node->type = builtin_types_.isize_type;
        } else if (num.Type == "usize") {
            node->type = builtin_types_.usize_type;
        } else if (num.Type == "anyint") {
            node->type = builtin_types_.any_integer_type;
        }
        break;
    }
    case TokenType::TRUE:
    case TokenType::FALSE:
        node->type = builtin_types_.bool_type;
        break;

    case TokenType::STRING: {
        auto str_primitive_type = std::make_shared<PrimitiveType>(TypeKind::STR);
        node->type = std::make_shared<ReferenceType>(str_primitive_type, false);
        break;
    }

    case TokenType::CHAR:
        node->type = builtin_types_.char_type;
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
    auto operand_type = node->right->type;

    if (!operand_type) {
        return nullptr;
    }

    switch (node->op.type) {
    case TokenType::MINUS:
        if (is_any_integer_type(operand_type->kind) && operand_type->kind != TypeKind::U32 &&
            operand_type->kind != TypeKind::USIZE) {
            node->type = operand_type;
        } else {
            error_reporter_.report_error(
                "Unary '+' and '-' operators can only be applied to integer types.", node->op.line);
            node->type = nullptr;
        }
        break;
    case TokenType::PLUS:
        if (is_any_integer_type(operand_type->kind)) {
            node->type = operand_type;
        } else {
            error_reporter_.report_error(
                "Unary '+' and '-' operators can only be applied to integer types.", node->op.line);
            node->type = nullptr;
        }
        break;
    case TokenType::BANG:
        if (operand_type->kind == TypeKind::BOOL) {
            node->type = builtin_types_.bool_type;
        } else {
            error_reporter_.report_error(
                "Logical NOT operator '!' can only be applied to boolean types.", node->op.line);
            node->type = nullptr;
        }
        break;

    case TokenType::STAR: {
        if (auto *ref_type = dynamic_cast<ReferenceType *>(operand_type.get())) {
            node->type = ref_type->referenced_type;
        } else {
            error_reporter_.report_error("Cannot dereference a non-reference type. Type '" +
                                             operand_type->to_string() +
                                             "' is not a pointer or reference.",
                                         node->op.line);
            node->type = nullptr;
        }
        break;
    }

    default:
        error_reporter_.report_error("Unsupported unary operator.", node->op.line);
        node->type = nullptr;
        break;
    }

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
    case TokenType::PERCENT:
    case TokenType::CARET:
    case TokenType::LESS_LESS:
    case TokenType::GREATER_GREATER:
    case TokenType::AMPERSAND:
    case TokenType::PIPE: {

        bool left_is_int = is_any_integer_type(left_type->kind);
        bool right_is_int = is_any_integer_type(right_type->kind);

        if (left_is_int && right_is_int) {
            if (is_concrete_integer(left_type->kind)) {
                if (is_concrete_integer(right_type->kind)) {
                    if (left_type->equals(right_type.get())) {
                        node->type = left_type;
                    } else {
                        error_reporter_.report_error(
                            "Mismatched integer types in binary operation. Both operands must be "
                            "of the same concrete integer type.",
                            node->op.line);
                        return nullptr;
                    }
                } else {
                    node->type = left_type;
                }
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
        } else if (left_type->kind == TypeKind::UNIT && right_type->kind == TypeKind::UNIT) {
            is_valid = true;
        } else if (left_type->kind == TypeKind::CHAR && right_type->kind == TypeKind::CHAR) {
            is_valid = true;
        } else if (left_type->kind == TypeKind::STR && right_type->kind == TypeKind::STR) {
            is_valid = true;
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

    if (!node->callee->type) {
        return nullptr;
    }

    if (node->callee->type->kind != TypeKind::FUNCTION) {
        error_reporter_.report_error("This expression is not callable.");
        return nullptr;
    }
    auto fn_type = std::dynamic_pointer_cast<FunctionType>(node->callee->type);

    bool is_method_call = (dynamic_cast<FieldAccessExpr *>(node->callee.get()) != nullptr);

    if (is_method_call) {

        if (fn_type->param_types.size() != node->arguments.size() + 1) {
            error_reporter_.report_error("Incorrect number of arguments for method. Expected " +
                                         std::to_string(fn_type->param_types.size() - 1) +
                                         ", but found " + std::to_string(node->arguments.size()));
            return nullptr;
        }

        if (auto *field_access = dynamic_cast<FieldAccessExpr *>(node->callee.get())) {
            auto object = field_access->object;
            if (!fn_type->param_types.empty()) {
                if (auto *self_ref_type =
                        dynamic_cast<ReferenceType *>(fn_type->param_types[0].get())) {
                    if (self_ref_type->is_mutable &&
                        (!object->resolved_symbol || !object->resolved_symbol->is_mutable)) {
                        error_reporter_.report_error(
                            "Cannot call mutable method on an immutable value.");
                    }
                }
            }
        }

        for (size_t i = 0; i < node->arguments.size(); ++i) {
            auto &arg_type = node->arguments[i]->type;

            auto &param_type = fn_type->param_types[i + 1];
            if (arg_type && !arg_type->equals(param_type.get())) {
                error_reporter_.report_error("Mismatched types. Expected argument type '" +
                                             param_type->to_string() + "' but found '" +
                                             arg_type->to_string() + "'.");
            }
        }

    } else {

        if (fn_type->param_types.size() != node->arguments.size()) {
            error_reporter_.report_error("Incorrect number of arguments for function. Expected " +
                                         std::to_string(fn_type->param_types.size()) +
                                         ", but found " + std::to_string(node->arguments.size()));
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
    if (node->condition->type && node->condition->type->kind != TypeKind::BOOL) {
        error_reporter_.report_error("Condition of 'if' expression must be of type 'bool'.");
    }

    node->then_branch->accept(this);
    auto then_type = node->then_branch->type;

    if (node->else_branch) {
        (*node->else_branch)->accept(this);
        auto else_type = (*node->else_branch)->type;

        if (then_type && else_type) {
            if (then_type->kind == TypeKind::NEVER) {
                node->type = else_type;

            } else if (else_type->kind == TypeKind::NEVER) {
                node->type = then_type;
            } else if (!then_type->equals(else_type.get())) {
                error_reporter_.report_error("'if' and 'else' have incompatible types. Expected '" +
                                             then_type->to_string() + "' but found '" +
                                             else_type->to_string() + "'.");
                node->type = nullptr;
            } else {
                node->type = then_type;
            }
        } else {
            error_reporter_.report_error("Cannot infer type of 'if' expression.");
        }
    } else {
        auto unit_type = std::make_shared<UnitType>();
        auto never_type = std::make_shared<NeverType>();
        if (then_type &&
            (!then_type->equals(unit_type.get()) && !then_type->equals(never_type.get()))) {
            error_reporter_.report_error("If expression without an 'else' branch must result in "
                                         "type '()' or '!', but found '" +
                                         then_type->to_string() + "'.");
        }
        node->type = unit_type;
    }

    if (node->else_branch) {
        node->return_over = node->then_branch->return_over && (*node->else_branch)->return_over;
    } else {
        node->return_over = false;
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

    auto new_object_type = object_type;
    if (auto *ref_type = dynamic_cast<ReferenceType *>(object_type.get())) {

        new_object_type = ref_type->referenced_type;
    }

    if (new_object_type->kind != TypeKind::ARRAY) {
        error_reporter_.report_error("Type '" + object_type->to_string() + "' cannot be indexed.");
        return nullptr;
    }
    auto array_type = std::dynamic_pointer_cast<ArrayType>(new_object_type);

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
    auto object_type = node->object->type;
    if (!object_type)
        return nullptr;

    std::string method_name = node->field.lexeme;

    auto method_symbol = object_type->members->lookup(method_name);

    if (method_symbol) {
        node->type = method_symbol->type;
        node->resolved_symbol = method_symbol;
        return nullptr;
    }

    auto base_type = object_type;
    if (auto ref_type = std::dynamic_pointer_cast<ReferenceType>(object_type)) {
        base_type = ref_type->referenced_type;
    }
    if (base_type->kind == TypeKind::ARRAY && method_name == "len") {
        auto usize_type = symbol_table_.lookup("usize")->type;
        std::vector<std::shared_ptr<Type>> len_param_types = {
            std::make_shared<ReferenceType>(object_type, false)};

        auto len_fn_type = std::make_shared<FunctionType>(usize_type, len_param_types);

        node->type = len_fn_type;

        node->resolved_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, len_fn_type);
        node->resolved_symbol->is_builtin = true;

        return nullptr;
    }

    error_reporter_.report_error("No method named '" + method_name + "' found for type '" +
                                     object_type->to_string() + "'.",
                                 node->field.line);
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(AssignmentExpr *node) {
    node->target->accept(this);
    node->value->accept(this);
    std::shared_ptr<Symbol> target_symbol = node->target->resolved_symbol;

    if (dynamic_cast<UnderscoreExpr *>(node->target.get())) {
        node->type = std::make_shared<UnitType>();
        return nullptr;
    }

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
        node->return_over = (*node->final_expr)->return_over;
    } else if (!node->statements.empty()) {
        node->return_over = node->statements.back()->return_over;
    } else {
        node->return_over = false;
    }

    if (node->return_over) {
        node->type = std::make_shared<NeverType>();
        return;
    }
    if (node->final_expr) {
        (*node->final_expr)->accept(this);
        node->type = (*node->final_expr)->type;
    } else {
        node->type = std::make_shared<UnitType>();
    }
}
void TypeCheckVisitor::visit(ExprStmt *node) {
    node->expression->accept(this);
    node->type = node->expression->type;
    node->return_over = node->expression->return_over;
}

void TypeCheckVisitor::visit(LetStmt *node) {
    if (!node->initializer) {
        return;
    }
    (*node->initializer)->accept(this);
    if (dynamic_cast<UnderscoreExpr *>((*node->initializer).get())) {
        error_reporter_.report_error(
            "Underscore `_` cannot be used as an initializer for a let binding.");
        return;
    }
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
    node->type = std::make_shared<NeverType>();
    node->return_over = true;
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

        auto actual_body_type = (*node->body)->type;

        if (current_return_type_ && actual_body_type &&
            !current_return_type_->equals(actual_body_type.get()) && !(*node->body)->return_over) {
            error_reporter_.report_error("Mismatched return type in function body. Expected '" +
                                         current_return_type_->to_string() + "' but found '" +
                                         actual_body_type->to_string() + "'.");
        }
    }
    if (current_function_symbol_ && current_function_symbol_->name == "main") {
        if (node->return_type && (*node->return_type)->resolved_type &&
            (*node->return_type)->resolved_type->kind != TypeKind::UNIT) {
            error_reporter_.report_error("The 'main' function must have a return type of '()'.");
        }
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
void TypeCheckVisitor::visit(IdentifierPattern *node) {}

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
    node->left->accept(this);
    auto left_symbol = node->left->resolved_symbol;
    if (!left_symbol) {
        error_reporter_.report_error("Undefined symbol in path expression.");
        return nullptr;
    }

    if (left_symbol->kind != Symbol::TYPE) {
        error_reporter_.report_error("Expected a type before `::`, but found a variable.");
        return nullptr;
    }

    auto right_name_opt = get_name_from_expr(node->right.get());
    if (!right_name_opt) {
        error_reporter_.report_error("Expected an identifier after `::`.");
        return nullptr;
    }

    auto assoc_fn_symbol = left_symbol->members->lookup(*right_name_opt);
    if (!assoc_fn_symbol) {
        error_reporter_.report_error("No function named '" + *right_name_opt +
                                     "' associated with type '" + left_symbol->name + "'.");
        return nullptr;
    }

    node->type = assoc_fn_symbol->type;
    node->resolved_symbol = assoc_fn_symbol;
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(BlockExpr *node) {
    if (node->block_stmt) {
        node->block_stmt->accept(this);
        node->type = node->block_stmt->type;
        node->return_over = node->block_stmt->return_over;
    }

    return nullptr;
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

void TypeCheckVisitor::visit(EnumDecl *node) {}

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