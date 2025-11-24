

#include "ir_generator.h"

/**
 * Generate IR for literal expressions.
 *
 * Handles:
 * - Integer literals: decimal, hex (0x), octal (0o), binary (0b)
 * - Boolean literals: true -> 1, false -> 0 (as i1)
 * - Character literals: converted to i32 Unicode scalar values
 * - String literals: TODO - global constant strings
 *
 * Features:
 * - Automatic base detection for numeric literals
 * - Type suffix removal (42i32 -> 42)
 * - Direct constant generation (no load instruction needed)
 *
 * @param node The literal expression AST node
 * @note Results are stored as constant values in expr_results_
 */
void IRGenerator::visit(LiteralExpr *node) {
    if (!node->type) {
        return;
    }

    std::string type_str = type_mapper_.map(node->type.get());
    std::string value;

    switch (node->literal.type) {
    case TokenType::NUMBER: {
        std::string lexeme = node->literal.lexeme;

        size_t suffix_pos = lexeme.find_first_of("iu");
        if (suffix_pos != std::string::npos) {
            lexeme = lexeme.substr(0, suffix_pos);
        }

        int base = 10;
        size_t start_pos = 0;

        if (lexeme.length() > 2 && lexeme[0] == '0') {
            if (lexeme[1] == 'x' || lexeme[1] == 'X') {
                base = 16;
                start_pos = 2;
            } else if (lexeme[1] == 'b' || lexeme[1] == 'B') {
                base = 2;
                start_pos = 2;
            } else if (lexeme[1] == 'o' || lexeme[1] == 'O') {
                base = 8;
                start_pos = 2;
            }
        }

        try {
            long long num_value = std::stoll(lexeme.substr(start_pos), nullptr, base);
            value = std::to_string(num_value);
        } catch (...) {
            value = "0";
        }
        break;
    }
    case TokenType::TRUE:
        value = "1";
        break;
    case TokenType::FALSE:
        value = "0";
        break;
    case TokenType::CHAR: {
        if (node->literal.lexeme.length() >= 3) {
            char c = node->literal.lexeme[1];
            value = std::to_string(static_cast<int>(static_cast<unsigned char>(c)));
        }
        break;
    }
    case TokenType::STRING:
        value = "null";
        break;
    default:
        value = "0";
        break;
    }

    store_expr_result(node, value);
}

/**
 * Generate IR for variable expressions.
 *
 * Variables can be:
 * - Local variables: Allocated on stack with alloca
 * - Function parameters: Allocated on entry, populated by caller
 * - Global variables: Defined at module level
 *
 * Loading behavior:
 * 1. Regular types (i32, bool):
 *    %value = load i32, i32* %var.addr
 *
 * 2. Aggregates in lvalue context (assignment target):
 *    Return pointer without loading
 *    Example: arr[0] = 5  (arr returns pointer)
 *
 * 3. Reference types (&T, &mut T):
 *    Load the pointer, not the pointee
 *    Example: let r = &x; (r is the pointer to x)
 *
 * Type resolution:
 * - Type comes from semantic analysis
 * - Used to generate correct LLVM type string
 *
 * @param node The variable expression AST node
 */
void IRGenerator::visit(VariableExpr *node) {
    std::string var_name = node->name.lexeme;

    VariableInfo *var_info = value_manager_.lookup_variable(var_name);

    if (var_info) {
        if (!node->type) {
            store_expr_result(node, "");
            return;
        }

        bool is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
        bool is_reference = (node->type->kind == TypeKind::REFERENCE);

        if (is_aggregate || is_reference || generating_lvalue_) {
            store_expr_result(node, var_info->alloca_name);
        } else {
            std::string type_str = type_mapper_.map(node->type.get());
            std::string loaded_value = emitter_.emit_load(type_str, var_info->alloca_name);
            store_expr_result(node, loaded_value);
        }
        return;
    }

    if (node->resolved_symbol && node->resolved_symbol->kind == Symbol::CONSTANT) {
        std::string const_name = node->name.lexeme;
        std::string type_str = type_mapper_.map(node->type.get());

        std::string loaded_value = emitter_.emit_load(type_str, "@" + const_name);
        store_expr_result(node, loaded_value);
        return;
    }

    store_expr_result(node, "");
}

/**
 * Generate IR for binary expressions.
 *
 * Operator categories:
 * - Arithmetic: + (add), - (sub), * (mul), / (div), % (rem)
 * - Comparison: == (eq), != (ne), < (lt), <= (le), > (gt), >= (ge)
 * - Logical: && (and - short-circuit), || (or - short-circuit)
 * - Bitwise: & (and), | (or), ^ (xor), << (shl), >> (shr/ashr)
 *
 * Optimizations:
 * - Constant folding: compute at compile-time if both operands are literals
 * - Short-circuit evaluation: && and || use conditional branches
 * - Signed/unsigned: automatic selection based on operand types
 *
 * @param node The binary expression AST node
 * @note Short-circuit logical operators delegated to visit_logical_binary_expr
 */
void IRGenerator::visit(BinaryExpr *node) {
    if (!node->type) {
        return;
    }

    if (node->op.type == TokenType::AMPERSAND_AMPERSAND || node->op.type == TokenType::PIPE_PIPE) {
        visit_logical_binary_expr(node);
        return;
    }

    node->left->accept(this);
    node->right->accept(this);

    std::string left_var = get_expr_result(node->left.get());
    std::string right_var = get_expr_result(node->right.get());

    if (left_var.empty() || right_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    auto left_literal = dynamic_cast<LiteralExpr *>(node->left.get());
    auto right_literal = dynamic_cast<LiteralExpr *>(node->right.get());

    if (left_literal && right_literal &&
        (left_literal->literal.type == TokenType::NUMBER ||
         left_literal->literal.type == TokenType::TRUE ||
         left_literal->literal.type == TokenType::FALSE) &&
        (right_literal->literal.type == TokenType::NUMBER ||
         right_literal->literal.type == TokenType::TRUE ||
         right_literal->literal.type == TokenType::FALSE)) {

        try {
            long long left_val = std::stoll(left_var);
            long long right_val = std::stoll(right_var);
            long long result_val = 0;
            bool can_fold = true;

            switch (node->op.type) {
            case TokenType::PLUS:
                result_val = left_val + right_val;
                break;
            case TokenType::MINUS:
                result_val = left_val - right_val;
                break;
            case TokenType::STAR:
                result_val = left_val * right_val;
                break;
            case TokenType::SLASH:
                if (right_val != 0)
                    result_val = left_val / right_val;
                else
                    can_fold = false;
                break;
            case TokenType::PERCENT:
                if (right_val != 0)
                    result_val = left_val % right_val;
                else
                    can_fold = false;
                break;
            case TokenType::LESS:
                result_val = (left_val < right_val) ? 1 : 0;
                break;
            case TokenType::LESS_EQUAL:
                result_val = (left_val <= right_val) ? 1 : 0;
                break;
            case TokenType::GREATER:
                result_val = (left_val > right_val) ? 1 : 0;
                break;
            case TokenType::GREATER_EQUAL:
                result_val = (left_val >= right_val) ? 1 : 0;
                break;
            case TokenType::EQUAL_EQUAL:
                result_val = (left_val == right_val) ? 1 : 0;
                break;
            case TokenType::BANG_EQUAL:
                result_val = (left_val != right_val) ? 1 : 0;
                break;
            case TokenType::AMPERSAND:
                result_val = left_val & right_val;
                break;
            case TokenType::PIPE:
                result_val = left_val | right_val;
                break;
            case TokenType::CARET:
                result_val = left_val ^ right_val;
                break;
            case TokenType::LESS_LESS:
                result_val = left_val << right_val;
                break;
            case TokenType::GREATER_GREATER:
                result_val = left_val >> right_val;
                break;
            default:
                can_fold = false;
            }

            if (can_fold) {
                store_expr_result(node, std::to_string(result_val));
                return;
            }
        } catch (...) {
        }
    }

    std::string type_str = type_mapper_.map(node->type.get());

    bool is_unsigned = false;
    if (node->left->type) {
        TypeKind kind = node->left->type->kind;
        is_unsigned = (kind == TypeKind::U32 || kind == TypeKind::USIZE);
    }

    std::string result;

    if (node->op.type == TokenType::EQUAL_EQUAL || node->op.type == TokenType::BANG_EQUAL ||
        node->op.type == TokenType::LESS || node->op.type == TokenType::LESS_EQUAL ||
        node->op.type == TokenType::GREATER || node->op.type == TokenType::GREATER_EQUAL) {
        std::string pred = token_to_icmp_pred(node->op, is_unsigned);

        std::string operand_type_str;
        if (node->left->type) {
            operand_type_str = type_mapper_.map(node->left->type.get());
        } else {
            operand_type_str = "i32";
        }

        result = emitter_.emit_icmp(pred, operand_type_str, left_var, right_var);
    } else {
        std::string ir_op = token_to_ir_op(node->op, is_unsigned);
        result = emitter_.emit_binary_op(ir_op, type_str, left_var, right_var);
    }

    store_expr_result(node, result);
}

/**
 * Generate IR for unary expressions.
 *
 * Operators:
 * - Negation (-x): Implemented as sub T 0, %x
 *   Example: -5 becomes sub i32 0, 5
 *
 * - Logical NOT (!x): Implemented as xor i1 %x, 1
 *   Example: !true becomes xor i1 1, 1 -> 0
 *
 * - Dereference (*ptr): Loads value through pointer
 *   Example: *ptr becomes load T, T* %ptr
 *
 * - Address-of (&x): Returns pointer to lvalue
 *   Sets generating_lvalue_ flag to prevent automatic load
 *   Example: &x returns %x.addr directly without load
 *
 * @param node The unary expression AST node
 */
void IRGenerator::visit(UnaryExpr *node) {
    if (!node->type) {
        return;
    }

    node->right->accept(this);
    std::string operand = get_expr_result(node->right.get());

    if (operand.empty()) {
        store_expr_result(node, "");
        return;
    }

    std::string type_str = type_mapper_.map(node->type.get());
    std::string result;

    switch (node->op.type) {
    case TokenType::MINUS:
        result = emitter_.emit_neg(type_str, operand);
        break;
    case TokenType::BANG:
        if (type_str == "i1") {
            result = emitter_.emit_not(operand);
        } else {
            result = emitter_.emit_binary_op("xor", type_str, operand, "-1");
        }
        break;
    case TokenType::STAR:
        if (node->type) {
            bool is_aggregate =
                (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
            if (is_aggregate) {
                result = operand;
            } else {
                result = emitter_.emit_load(type_str, operand);
            }
        } else {
            result = emitter_.emit_load(type_str, operand);
        }
        break;
    default:
        result = operand;
        break;
    }

    store_expr_result(node, result);
}

// Generate IR for short-circuit logical binary expressions (&& and ||).
void IRGenerator::visit_logical_binary_expr(BinaryExpr *node) {
    bool is_or = (node->op.type == TokenType::PIPE_PIPE);

    static int logical_counter = 0;
    int current = logical_counter++;

    std::string rhs_label = (is_or ? "or.rhs." : "and.rhs.") + std::to_string(current);
    std::string end_label = (is_or ? "or.end." : "and.end.") + std::to_string(current);

    node->left->accept(this);
    std::string left_var = get_expr_result(node->left.get());

    if (left_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    std::string left_block = current_block_label_;

    if (is_or) {
        emitter_.emit_cond_br(left_var, end_label, rhs_label);
    } else {
        emitter_.emit_cond_br(left_var, rhs_label, end_label);
    }

    begin_block(rhs_label);
    current_block_terminated_ = false;

    node->right->accept(this);
    std::string right_var = get_expr_result(node->right.get());

    if (right_var.empty()) {
        emitter_.emit_br(end_label);
        begin_block(end_label);
        current_block_terminated_ = false;
        store_expr_result(node, left_var);
        return;
    }

    std::string right_block = current_block_label_;

    if (!current_block_terminated_) {
        emitter_.emit_br(end_label);
    }

    begin_block(end_label);
    current_block_terminated_ = false;

    std::vector<std::pair<std::string, std::string>> phi_incoming;
    phi_incoming.push_back({left_var, left_block});
    phi_incoming.push_back({right_var, right_block});

    std::string result = emitter_.emit_phi("i1", phi_incoming);
    store_expr_result(node, result);
}

/**
 * Generate IR for function call expressions.
 *
 * Handles:
 * - Built-in functions: print, scan, exit (special handling)
 * - User-defined functions: regular call instruction
 * - Method calls: impl block methods (self parameter)
 * - SRET optimization: large struct returns via pointer parameter
 *
 * Argument passing strategy:
 * - Scalar types: pass by value (loaded if from variable)
 * - Aggregate types (arrays/structs): pass pointer directly
 * - References: pass pointer value
 *
 * Return value handling:
 * - Void functions: no return value
 * - Scalar returns: use call result directly
 * - Aggregate returns: SRET or direct pointer
 *
 * @param node The call expression AST node
 */
void IRGenerator::visit(CallExpr *node) {
    std::vector<std::pair<std::string, std::string>> args;

    for (const auto &arg : node->arguments) {
        arg->accept(this);
        std::string arg_value = get_expr_result(arg.get());

        if (arg_value.empty()) {
            continue;
        }

        if (!arg->type) {
            continue;
        }

        std::string arg_type_str = type_mapper_.map(arg->type.get());

        bool is_aggregate =
            (arg->type->kind == TypeKind::ARRAY || arg->type->kind == TypeKind::STRUCT);
        bool is_reference = (arg->type->kind == TypeKind::REFERENCE);

        if (is_aggregate || is_reference) {
            if (is_reference) {
                auto ref_type = std::dynamic_pointer_cast<ReferenceType>(arg->type);
                if (ref_type && ref_type->referenced_type) {
                    std::string actual_type_str = type_mapper_.map(ref_type->referenced_type.get());
                    args.push_back({actual_type_str + "*", arg_value});
                } else {
                    args.push_back({arg_type_str + "*", arg_value});
                }
            } else {
                args.push_back({arg_type_str + "*", arg_value});
            }
        } else {
            args.push_back({arg_type_str, arg_value});
        }
    }

    std::string func_name;
    std::vector<std::pair<std::string, std::string>> self_args;

    if (auto var_expr = dynamic_cast<VariableExpr *>(node->callee.get())) {
        func_name = var_expr->name.lexeme;
    } else if (auto path_expr = dynamic_cast<PathExpr *>(node->callee.get())) {

        std::string type_name;
        std::string method_name;

        if (auto left_var = dynamic_cast<VariableExpr *>(path_expr->left.get())) {
            type_name = left_var->name.lexeme;
        }

        if (auto right_var = dynamic_cast<VariableExpr *>(path_expr->right.get())) {
            method_name = right_var->name.lexeme;
        }

        if (!type_name.empty() && !method_name.empty()) {
            func_name = type_name + "_" + method_name;
        } else {
            store_expr_result(node, "");
            return;
        }
    } else if (auto field_expr = dynamic_cast<FieldAccessExpr *>(node->callee.get())) {

        field_expr->object->accept(this);
        std::string obj_ptr = get_expr_result(field_expr->object.get());

        if (obj_ptr.empty()) {
            store_expr_result(node, "");
            return;
        }

        std::string type_name;
        if (field_expr->object->type) {
            if (auto struct_type =
                    std::dynamic_pointer_cast<StructType>(field_expr->object->type)) {
                type_name = struct_type->name;
            } else if (field_expr->object->type->kind == TypeKind::REFERENCE) {
                auto ref_type = std::dynamic_pointer_cast<ReferenceType>(field_expr->object->type);
                if (ref_type && ref_type->referenced_type) {
                    if (auto struct_type =
                            std::dynamic_pointer_cast<StructType>(ref_type->referenced_type)) {
                        type_name = struct_type->name;
                    }
                }
            } else if (field_expr->object->type->kind == TypeKind::RAW_POINTER) {
                auto ptr_type = std::dynamic_pointer_cast<RawPointerType>(field_expr->object->type);
                if (ptr_type && ptr_type->pointee_type) {
                    if (auto struct_type =
                            std::dynamic_pointer_cast<StructType>(ptr_type->pointee_type)) {
                        type_name = struct_type->name;
                    }
                }
            }
        }

        std::string method_name = field_expr->field.lexeme;

        if (type_name.empty() || method_name.empty()) {
            store_expr_result(node, "");
            return;
        }

        func_name = type_name + "_" + method_name;

        std::string obj_type_str;
        if (field_expr->object->type->kind == TypeKind::REFERENCE) {
            auto ref_type = std::dynamic_pointer_cast<ReferenceType>(field_expr->object->type);
            if (ref_type && ref_type->referenced_type) {
                obj_type_str = type_mapper_.map(ref_type->referenced_type.get()) + "*";
            }
        } else {
            obj_type_str = type_mapper_.map(field_expr->object->type.get()) + "*";
        }
        self_args.push_back({obj_type_str, obj_ptr});
    } else {
        store_expr_result(node, "");
        return;
    }

    if (handle_builtin_function(node, func_name, args)) {
        return;
    }

    std::string ret_type_str = "void";
    bool ret_is_aggregate = false;
    bool use_sret = false;

    if (node->type) {
        ret_type_str = type_mapper_.map(node->type.get());
        ret_is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);

        if (should_use_sret_optimization(func_name, node->type.get())) {
            use_sret = true;
        }
    }

    std::vector<std::pair<std::string, std::string>> all_args;

    std::string sret_alloca;
    if (use_sret) {
        sret_alloca = emitter_.emit_alloca(ret_type_str);
        all_args.push_back({ret_type_str + "*", sret_alloca});
    }

    all_args.insert(all_args.end(), self_args.begin(), self_args.end());
    all_args.insert(all_args.end(), args.begin(), args.end());

    if (use_sret) {
        emitter_.emit_call_void(func_name, all_args);
        store_expr_result(node, sret_alloca);
    } else if (ret_type_str == "void") {
        emitter_.emit_call_void(func_name, all_args);
        store_expr_result(node, "");
    } else {
        std::string result = emitter_.emit_call(ret_type_str, func_name, all_args);

        if (ret_is_aggregate) {
            std::string alloca_ptr = emitter_.emit_alloca(ret_type_str);
            emitter_.emit_store(ret_type_str, result, alloca_ptr);
            store_expr_result(node, alloca_ptr);
        } else {
            store_expr_result(node, result);
        }
    }
}

/**
 * Generate IR for assignment expressions.
 *
 * Syntax: lvalue = rvalue
 *
 * Process:
 * 1. Evaluate right-hand side (value expression)
 * 2. Get pointer to left-hand side (lvalue address)
 * 3. Store value to pointer
 *
 * Examples:
 * - Simple variable: x = 5
 *   store i32 5, i32* %x.addr
 *
 * - Array element: arr[i] = 10
 *   %ptr = getelementptr [10 x i32], [10 x i32]* %arr, i32 0, i32 %i
 *   store i32 10, i32* %ptr
 *
 * - Struct field: point.x = 3
 *   %ptr = getelementptr %Point, %Point* %point, i32 0, i32 0
 *   store i32 3, i32* %ptr
 *
 * Return value:
 * - Assignment is an expression that returns the assigned value
 * - Enables chaining: x = y = 5
 *
 * @param node The assignment expression AST node
 */
void IRGenerator::visit(AssignmentExpr *node) {

    node->value->accept(this);
    std::string value_var = get_expr_result(node->value.get());

    if (value_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    if (auto var_expr = dynamic_cast<VariableExpr *>(node->target.get())) {
        std::string var_name = var_expr->name.lexeme;
        VariableInfo *var_info = value_manager_.lookup_variable(var_name);

        if (!var_info) {
            store_expr_result(node, "");
            return;
        }

        if (!var_info->is_mutable) {
            store_expr_result(node, "");
            return;
        }

        if (node->value->type && var_expr->type) {
            std::string value_type_str = type_mapper_.map(node->value->type.get());
            std::string target_type_str = type_mapper_.map(var_expr->type.get());

            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                size_t size = get_type_size(node->value->type.get());
                std::string ptr_type = target_type_str + "*";
                emitter_.emit_memcpy(var_info->alloca_name, value_var, size, ptr_type);
            } else {
                if (node->value->type->kind == TypeKind::BOOL &&
                    (var_expr->type->kind == TypeKind::I32 ||
                     var_expr->type->kind == TypeKind::USIZE)) {
                    value_to_store = emitter_.emit_zext("i1", value_to_store, target_type_str);
                    value_type_str = target_type_str;
                }

                emitter_.emit_store(target_type_str, value_to_store, var_info->alloca_name);
            }
        }

        store_expr_result(node, "");
    } else if (auto index_expr = dynamic_cast<IndexExpr *>(node->target.get())) {
        bool old_flag = generating_lvalue_;
        generating_lvalue_ = true;
        index_expr->accept(this);
        generating_lvalue_ = old_flag;

        std::string elem_ptr = get_expr_result(index_expr);

        if (!elem_ptr.empty() && node->value->type) {
            std::string type_str = type_mapper_.map(node->value->type.get());

            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                size_t size = get_type_size(node->value->type.get());
                std::string ptr_type = type_str + "*";
                emitter_.emit_memcpy(elem_ptr, value_var, size, ptr_type);
            } else {
                emitter_.emit_store(type_str, value_to_store, elem_ptr);
            }
        }

        store_expr_result(node, "");
    } else if (auto field_expr = dynamic_cast<FieldAccessExpr *>(node->target.get())) {
        bool old_flag = generating_lvalue_;
        generating_lvalue_ = true;
        field_expr->accept(this);
        generating_lvalue_ = old_flag;

        std::string field_ptr = get_expr_result(field_expr);

        if (!field_ptr.empty() && node->value->type) {
            std::string type_str = type_mapper_.map(node->value->type.get());

            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                size_t size = get_type_size(node->value->type.get());
                std::string ptr_type = type_str + "*";
                emitter_.emit_memcpy(field_ptr, value_var, size, ptr_type);
            } else {
                emitter_.emit_store(type_str, value_to_store, field_ptr);
            }
        }

        store_expr_result(node, "");
    } else if (auto unary_expr = dynamic_cast<UnaryExpr *>(node->target.get())) {
        if (unary_expr->op.type == TokenType::STAR) {
            unary_expr->right->accept(this);
            std::string ptr_value = get_expr_result(unary_expr->right.get());

            if (!ptr_value.empty() && node->value->type) {
                std::string value_type_str = type_mapper_.map(node->value->type.get());
                std::string value_to_store = value_var;

                bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                           node->value->type->kind == TypeKind::STRUCT);

                if (value_is_aggregate) {
                    size_t size = get_type_size(node->value->type.get());
                    std::string ptr_type = value_type_str + "*";
                    emitter_.emit_memcpy(ptr_value, value_var, size, ptr_type);
                } else {
                    emitter_.emit_store(value_type_str, value_to_store, ptr_value);
                }
            }
        }

        store_expr_result(node, "");
    } else {
        store_expr_result(node, "");
    }
}

/**
 * Generate IR for grouping expressions (parentheses).
 *
 * Purpose: Parentheses control evaluation order and precedence
 *
 * Examples:
 * - (x + y) * z
 * - (a && b) || c
 * - return (value);
 *
 * IR generation:
 * - Simply forward to inner expression
 * - Parentheses affect AST structure, not IR
 * - No additional IR code generated
 *
 * Why parentheses matter:
 * - Change operator precedence: (a + b) * c vs a + (b * c)
 * - Group operands for clarity: if ((x > 0) && (y < 10))
 * - Return complex expressions: return (x + y);
 *
 * @param node The grouping expression AST node
 * @note This is essentially a pass-through, no IR emitted
 */
void IRGenerator::visit(GroupingExpr *node) {
    if (node->expression) {
        node->expression->accept(this);

        std::string result = get_expr_result(node->expression.get());
        store_expr_result(node, result);
    }
}

/**
 * Generate IR for block expressions.
 *
 * Syntax:
 *   {
 *       statement1;
 *       statement2;
 *       final_expression  // Optional, no semicolon
 *   }
 *
 * Value semantics:
 * - Statements execute sequentially
 * - Final expression (if present) becomes block's value
 * - Without final expr, block evaluates to unit ()
 *
 * Examples:
 *   let x = {
 *       let temp = 5;
 *       temp * 2  // Returns 10
 *   };
 *
 *   if condition {
 *       do_something();
 *       42  // Block returns 42
 *   } else {
 *       0   // Block returns 0
 *   }
 *
 * IR generation:
 * - Execute all statements in order
 * - Propagate final expression's result
 * - Track aggregate value loads for optimization
 *
 * @param node The block expression AST node
 */
void IRGenerator::visit(BlockExpr *node) {
    if (node->block_stmt) {
        node->block_stmt->accept(this);

        if (node->block_stmt->final_expr.has_value()) {
            auto final_expr = node->block_stmt->final_expr.value();
            if (final_expr) {
                std::string result = get_expr_result(final_expr.get());
                store_expr_result(node, result);

                if (loaded_aggregate_results_.find(final_expr.get()) !=
                    loaded_aggregate_results_.end()) {
                    loaded_aggregate_results_.insert(node);
                }
                return;
            }
        }
    }

    store_expr_result(node, "");
}

/**
 * Generate IR for type cast expressions (as operator).
 *
 * Syntax: expression as TargetType
 *
 * Supported conversions:
 * 1. Integer widening/narrowing:
 *    - Signed: sext (sign-extend) or trunc
 *      Example: i32 to i64 -> sext i32 %x to i64
 *    - Unsigned: zext (zero-extend) or trunc
 *      Example: u8 to u32 -> zext i8 %x to i32
 *
 * 2. Sign changes (no IR instruction needed):
 *    - i32 to u32, u32 to i32
 *    - Bitwise identical, only semantic difference
 *
 * 3. Bool to integer:
 *    - zext i1 to i32 (false->0, true->1)
 *
 * 4. Integer to bool:
 *    - icmp ne i32 %x, 0 (0->false, non-zero->true)
 *
 * 5. Pointer casts:
 *    - bitcast T* to U*
 *
 * @param node The type cast expression AST node
 */
void IRGenerator::visit(AsExpr *node) {

    node->expression->accept(this);
    std::string source_value = get_expr_result(node->expression.get());

    if (source_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    auto source_type = node->expression->type;
    auto target_type = node->type;

    if (!source_type || !target_type) {
        store_expr_result(node, "");
        return;
    }

    std::string source_llvm_type = type_mapper_.map(source_type.get());
    std::string target_llvm_type = type_mapper_.map(target_type.get());

    if (source_llvm_type == target_llvm_type) {
        store_expr_result(node, source_value);
        return;
    }

    int source_bits = get_integer_bits(source_type->kind);
    int target_bits = get_integer_bits(target_type->kind);

    std::string result;

    if (source_bits == target_bits) {
        result = source_value;
    } else if (source_bits < target_bits) {
        bool is_signed =
            (source_type->kind == TypeKind::I32 || source_type->kind == TypeKind::ISIZE);

        if (is_signed) {
            result = emitter_.emit_sext(source_llvm_type, source_value, target_llvm_type);
        } else {
            result = emitter_.emit_zext(source_llvm_type, source_value, target_llvm_type);
        }
    } else {
        result = emitter_.emit_trunc(source_llvm_type, source_value, target_llvm_type);
    }

    store_expr_result(node, result);
}

/**
 * Generate IR for reference expressions (&expr).
 *
 * Purpose: Take address of an lvalue
 *
 * Syntax: &variable, &arr[i], &struct.field
 *
 * Process:
 * 1. Set generating_lvalue_ flag to true
 * 2. Visit inner expression
 * 3. Inner expression returns pointer instead of value
 * 4. Pass pointer through as result
 *
 * Examples:
 * - &x: Returns %x.addr (pointer to x)
 * - &arr[i]: Returns GEP result pointer
 * - &point.x: Returns field pointer from GEP
 *
 * Type transformation:
 * - Input expression type: T
 * - Output type: &T (pointer to T)
 *
 * Use cases:
 * - Pass by reference to functions
 * - Store references in structs
 * - Take address for unsafe operations
 *
 * @param node The reference expression AST node
 */
void IRGenerator::visit(ReferenceExpr *node) {

    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true;
    node->expression->accept(this);
    generating_lvalue_ = was_generating_lvalue;

    std::string value = get_expr_result(node->expression.get());

    if (value.empty()) {
        store_expr_result(node, "");
        return;
    }

    if (!node->type && node->expression->type) {
        node->type = std::make_shared<ReferenceType>(node->expression->type, false);
    }

    if (node->expression->type) {
        bool is_aggregate = (node->expression->type->kind == TypeKind::ARRAY ||
                             node->expression->type->kind == TypeKind::STRUCT);

        if (is_aggregate) {
            store_expr_result(node, value);
        } else {
            store_expr_result(node, value);
        }
    } else {
        store_expr_result(node, value);
    }
}

/**
 * Generate IR for compound assignment expressions.
 *
 * Operators: +=, -=, *=, /=, %=
 *
 * Equivalent transformation:
 *   x += y  becomes  x = x + y
 *   arr[i] *= 5  becomes  arr[i] = arr[i] * 5
 *
 * Process:
 * 1. Evaluate left side to get pointer (lvalue)
 * 2. Load current value from pointer
 * 3. Evaluate right side expression
 * 4. Perform binary operation (add/sub/mul/div/rem)
 * 5. Store result back to pointer
 *
 * Example IR for x += 5:
 *   %1 = load i32, i32* %x.addr
 *   %2 = add i32 %1, 5
 *   store i32 %2, i32* %x.addr
 *
 * Works with:
 * - Simple variables: x += 5
 * - Array elements: arr[i] += 5
 * - Struct fields: point.x += 5
 * - Dereferences: *ptr += 5
 *
 * @param node The compound assignment expression AST node
 */
void IRGenerator::visit(CompoundAssignmentExpr *node) {

    std::string target_ptr;
    std::string target_type_str;

    if (auto var_expr = dynamic_cast<VariableExpr *>(node->target.get())) {
        std::string var_name = var_expr->name.lexeme;
        VariableInfo *var_info = value_manager_.lookup_variable(var_name);

        if (!var_info) {
            store_expr_result(node, "");
            return;
        }

        target_ptr = var_info->alloca_name;
        target_type_str = var_info->type_str;
        if (!target_type_str.empty() && target_type_str.back() == '*') {
            target_type_str.pop_back();
        }
    } else if (auto index_expr = dynamic_cast<IndexExpr *>(node->target.get())) {
        bool was_generating_lvalue = generating_lvalue_;
        generating_lvalue_ = true;
        index_expr->accept(this);
        generating_lvalue_ = was_generating_lvalue;

        target_ptr = get_expr_result(index_expr);

        if (target_ptr.empty() || !index_expr->type) {
            store_expr_result(node, "");
            return;
        }

        target_type_str = type_mapper_.map(index_expr->type.get());
    } else if (auto field_expr = dynamic_cast<FieldAccessExpr *>(node->target.get())) {
        bool was_generating_lvalue = generating_lvalue_;
        generating_lvalue_ = true;
        field_expr->accept(this);
        generating_lvalue_ = was_generating_lvalue;

        target_ptr = get_expr_result(field_expr);

        if (target_ptr.empty() || !field_expr->type) {
            store_expr_result(node, "");
            return;
        }

        target_type_str = type_mapper_.map(field_expr->type.get());
    } else if (auto unary_expr = dynamic_cast<UnaryExpr *>(node->target.get())) {
        if (unary_expr->op.type == TokenType::STAR) {
            unary_expr->right->accept(this);
            target_ptr = get_expr_result(unary_expr->right.get());

            if (target_ptr.empty() || !unary_expr->type) {
                store_expr_result(node, "");
                return;
            }

            target_type_str = type_mapper_.map(unary_expr->type.get());
        } else {
            store_expr_result(node, "");
            return;
        }
    } else {
        store_expr_result(node, "");
        return;
    }

    std::string current_value = emitter_.emit_load(target_type_str, target_ptr);

    node->value->accept(this);
    std::string rhs_value = get_expr_result(node->value.get());

    if (rhs_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    std::string result;
    std::string ir_op;

    switch (node->op.type) {
    case TokenType::PLUS_EQUAL:
        ir_op = "add";
        break;
    case TokenType::MINUS_EQUAL:
        ir_op = "sub";
        break;
    case TokenType::STAR_EQUAL:
        ir_op = "mul";
        break;
    case TokenType::SLASH_EQUAL:
        ir_op = "sdiv";
        break;
    case TokenType::PERCENT_EQUAL:
        ir_op = "srem";
        break;
    case TokenType::AMPERSAND_EQUAL:
        ir_op = "and";
        break;
    case TokenType::PIPE_EQUAL:
        ir_op = "or";
        break;
    case TokenType::CARET_EQUAL:
        ir_op = "xor";
        break;
    case TokenType::LESS_LESS_EQUAL:
        ir_op = "shl";
        break;
    case TokenType::GREATER_GREATER_EQUAL:
        ir_op = "ashr";
        break;
    default:
        store_expr_result(node, "");
        return;
    }

    result = emitter_.emit_binary_op(ir_op, target_type_str, current_value, rhs_value);

    emitter_.emit_store(target_type_str, result, target_ptr);

    store_expr_result(node, "");
}

void IRGenerator::visit(UnderscoreExpr *node) {}
void IRGenerator::visit(UnitExpr *node) {}
void IRGenerator::visit(TupleExpr *node) {}
void IRGenerator::visit(MatchExpr *node) {}
void IRGenerator::visit(PathExpr *node) {}
