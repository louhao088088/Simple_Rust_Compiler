// type_check.cpp

#include "semantic.h"

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

    if (array_element_type->kind == TypeKind::ANY_INTEGER) {
        for (const auto &element : node->elements) {
            if (is_concrete_integer(element->type->kind)) {
                array_element_type = element->type;
                break;
            }
        }
    }

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
    if (node->resolved_symbol && node->resolved_symbol->kind == Symbol::VARIABLE) {
        bool binding_is_mut = node->resolved_symbol->is_mutable;
        bool type_is_mut_ref = false;
        if (auto *ref_type = dynamic_cast<ReferenceType *>(node->type.get())) {
            type_is_mut_ref = ref_type->is_mutable;
        }
        node->is_mutable_lvalue = binding_is_mut || type_is_mut_ref;
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
            node->is_mutable_lvalue = ref_type->is_mutable;
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

bool is_raw_pointer(std::shared_ptr<Type> t) { return t && t->kind == TypeKind::RAW_POINTER; }

bool is_size_integer(std::shared_ptr<Type> t) {
    return t && (t->kind == TypeKind::ISIZE || t->kind == TypeKind::USIZE ||
                 t->kind == TypeKind::ANY_INTEGER);
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
    case TokenType::PLUS: {
        if (is_raw_pointer(left_type) && is_size_integer(right_type)) {
            node->type = node->left->type;
            return nullptr;
        }
        if (is_size_integer(left_type) && is_raw_pointer(right_type)) {
            node->type = node->right->type;
            return nullptr;
        }
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
    case TokenType::MINUS: {
        if (is_raw_pointer(left_type) && is_size_integer(right_type)) {
            node->type = node->left->type;
            return nullptr;
        }
        if (is_raw_pointer(left_type) && is_raw_pointer(right_type)) {
            if (left_type->equals(right_type.get())) {

                node->type = builtin_types_.isize_type;
                return nullptr;
            }
        }
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
bool is_compatible(Type *arg_type, Type *param_type) {
    if (!arg_type || !param_type)
        return false;

    if (arg_type->equals(param_type)) {
        return true;
    }

    if (auto *param_ref = dynamic_cast<const ReferenceType *>(param_type)) {
        if (auto *arg_ref = dynamic_cast<const ReferenceType *>(arg_type)) {
            if (is_compatible(arg_ref->referenced_type.get(), param_ref->referenced_type.get())) {
                if (!param_ref->is_mutable || arg_ref->is_mutable) {
                    return true;
                }
            }
        }
    }

    return false;
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
                    if (self_ref_type->is_mutable && (!object->is_mutable_lvalue)) {
                        error_reporter_.report_error(
                            "Cannot call mutable method on an immutable value.");
                    }
                }
            }
        }

        for (size_t i = 0; i < node->arguments.size(); ++i) {
            auto &arg_type = node->arguments[i]->type;

            auto &param_type = fn_type->param_types[i + 1];
            if (arg_type && !is_compatible(arg_type.get(), param_type.get())) {
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
            if (arg_type && !is_compatible(arg_type.get(), param_type.get())) {
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
            if (current_function_symbol_ && !current_function_symbol_->is_main) {
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

        node->type = std::make_shared<NeverType>();
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
        node->type = std::make_shared<NeverType>();
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

    node->is_mutable_lvalue = node->object->is_mutable_lvalue;
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    auto object_type = node->object->type;
    if (!object_type)
        return nullptr;

    auto effective_type = object_type;
    if (auto *ref_type = dynamic_cast<ReferenceType *>(object_type.get())) {
        effective_type = ref_type->referenced_type;
    }

    std::string method_name = node->field.lexeme;

    auto method_symbol = effective_type->members->lookup_value(method_name);

    if (method_symbol) {

        if (method_symbol->kind == Symbol::VARIABLE) {

            node->type = method_symbol->type;
            node->is_mutable_lvalue = node->object->is_mutable_lvalue;
        } else if (method_symbol->kind == Symbol::FUNCTION) {
            node->type = method_symbol->type;
            node->is_mutable_lvalue = false;
        } else {
            error_reporter_.report_error("Member '" + method_name + "' is not a field or method.",
                                         node->field.line);
            return nullptr;
        }
        node->resolved_symbol = method_symbol;
        return nullptr;
    }

    if (effective_type->kind == TypeKind::ARRAY && method_name == "len") {

        auto usize_symbol = symbol_table_.lookup_type("usize");
        if (!usize_symbol) {

            error_reporter_.report_error("FATAL: Built-in type 'usize' not found.",
                                         node->field.line);
            return nullptr;
        }
        auto usize_type = usize_symbol->type;

        std::vector<std::shared_ptr<Type>> len_param_types = {
            std::make_shared<ReferenceType>(object_type, false)};

        auto len_fn_type = std::make_shared<FunctionType>(usize_type, len_param_types);
        node->type = len_fn_type;
        node->resolved_symbol = std::make_shared<Symbol>("len", Symbol::FUNCTION, len_fn_type);
        node->resolved_symbol->is_builtin = true;

        return nullptr;
    }

    error_reporter_.report_error("No field or method named '" + method_name + "' found for type '" +
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

    if (!node->target->is_mutable_lvalue) {
        error_reporter_.report_error(
            "Invalid left-hand side of assignment. Target is not mutable.");
    }

    if (node->target->type && node->value->type) {
        if (node->value->type->kind == TypeKind::ANY_INTEGER &&
            (node->target->type->kind == TypeKind::I32 ||
             node->target->type->kind == TypeKind::U32 ||
             node->target->type->kind == TypeKind::ISIZE ||
             node->target->type->kind == TypeKind::USIZE)) {

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
    auto target_type = node->target->type;
    auto value_type = node->value->type;

    if (!target_type || !value_type) {
        return nullptr;
    }

    if (!node->target->is_mutable_lvalue) {
        error_reporter_.report_error(
            "Invalid left-hand side of assignment. Target is not mutable.");
    }

    bool operation_is_valid = false;
    switch (node->op.type) {
    case TokenType::PLUS_EQUAL:
    case TokenType::MINUS_EQUAL:
    case TokenType::STAR_EQUAL:
    case TokenType::SLASH_EQUAL:
    case TokenType::PERCENT_EQUAL:
    case TokenType::CARET_EQUAL:
    case TokenType::LESS_LESS_EQUAL:
    case TokenType::GREATER_GREATER_EQUAL:
    case TokenType::AMPERSAND_EQUAL:
    case TokenType::PIPE_EQUAL: {
        if (is_any_integer_type(target_type->kind) && is_any_integer_type(value_type->kind)) {
            operation_is_valid = true;
        }
        break;
    }
    default:
        operation_is_valid = false;
        break;
    }

    if (!operation_is_valid) {
        error_reporter_.report_error(
            "Cannot apply compound assignment operator '" + node->op.lexeme + "' to types '" +
                target_type->to_string() + "' and '" + value_type->to_string() + "'.",
            node->op.line);
    }

    node->type = std::make_shared<UnitType>();

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
        if (auto final_expr = std::dynamic_pointer_cast<UnderscoreExpr>(*node->final_expr)) {
            error_reporter_.report_error("Underscore `_` cannot be used as a final expression.");
            node->type = nullptr;
            return;
        }
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
    if (current_function_symbol_ && current_function_symbol_->is_main) {
        if (node->return_type && (*node->return_type)->resolved_type &&
            (*node->return_type)->resolved_type->kind != TypeKind::UNIT) {
            error_reporter_.report_error("The 'main' function must have a return type of '()'.");
        }
    }
    if (current_function_symbol_ && current_function_symbol_->is_main) {
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
    auto struct_symbol = node->resolved_symbol;
    if (!struct_symbol || !struct_symbol->type || struct_symbol->type->kind != TypeKind::STRUCT) {
        error_reporter_.report_error("Undefined struct type in struct initializer.");
        return nullptr;
    }
    auto struct_type = std::static_pointer_cast<StructType>(struct_symbol->type);

    std::set<std::string> provided_fields;
    for (const auto &field_init : node->fields) {
        provided_fields.insert(field_init->name.lexeme);
    }

    for (const auto &pair : struct_type->fields) {
        const std::string &expected_field_name = pair.first;
        if (provided_fields.find(expected_field_name) == provided_fields.end()) {
            error_reporter_.report_error("Missing field '" + expected_field_name +
                                         "' in initializer for struct '" + struct_type->name +
                                         "'.");
        }
    }

    for (const auto &field_name : provided_fields) {
        if (struct_type->fields.find(field_name) == struct_type->fields.end()) {
            error_reporter_.report_error("Struct '" + struct_type->name + "' has no field named '" +
                                         field_name + "'.");
        }
    }

    if (provided_fields.size() != struct_type->fields.size()) {
        return nullptr;
    }

    for (const auto &field_init : node->fields) {
        field_init->value->accept(this);
        auto actual_value_type = field_init->value->type;

        auto expected_field_type = struct_type->fields[field_init->name.lexeme];

        if (actual_value_type && expected_field_type &&
            !is_compatible(actual_value_type.get(), expected_field_type.get())) {
            error_reporter_.report_error("Mismatched types for field '" + field_init->name.lexeme +
                                         "'. Expected type '" + expected_field_type->to_string() +
                                         "' but found '" + actual_value_type->to_string() + "'.");
        }
    }

    node->type = struct_type;

    return nullptr;
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
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(AsExpr *node) {
    node->expression->accept(this);
    node->target_type->accept(this);

    auto source_type = node->expression->type;
    auto target_type = node->target_type->resolved_type;

    if (!source_type || !target_type) {
        node->type = nullptr;
        return nullptr;
    }

    if (target_type->kind == TypeKind::RAW_POINTER) {

        if (source_type->kind == TypeKind::REFERENCE) {

            node->type = target_type;
            return nullptr;
        }
    }

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

        arm->pattern->accept(this);
        if (arm->guard)
            (*arm->guard)->accept(this);
        arm->body->accept(this);
    }
    return nullptr;
}

std::shared_ptr<Symbol> TypeCheckVisitor::visit(PathExpr *node) {
    auto resolved_symbol = node->resolved_symbol;

    if (!resolved_symbol) {

        error_reporter_.report_error(
            "Internal error: Path expression has no resolved symbol in type checking.");
        return nullptr;
    }

    if (resolved_symbol->kind == Symbol::VARIANT || resolved_symbol->kind == Symbol::FUNCTION) {
        node->type = resolved_symbol->type;
    } else {
        error_reporter_.report_error(
            "Path expression does not resolve to a function or enum variant.");
        node->type = nullptr;
    }

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

void TypeCheckVisitor::visit(ItemStmt *node) { node->item->accept(this); }

void TypeCheckVisitor::visit(PathTypeNode *node) { node->path->accept(this); }

void TypeCheckVisitor::visit(RawPointerTypeNode *node) { node->pointee_type->accept(this); }

void TypeCheckVisitor::visit(ReferenceTypeNode *node) { node->referenced_type->accept(this); }

void TypeCheckVisitor::visit(SliceTypeNode *node) { node->element_type->accept(this); }

void TypeCheckVisitor::visit(SelfTypeNode *node) {}

void TypeCheckVisitor::visit(StructDecl *node) {}

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