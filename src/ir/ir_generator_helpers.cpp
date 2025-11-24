#include "ir_generator.h"

std::string IRGenerator::get_expr_result(Expr *expr) {
    auto it = expr_results_.find(expr);
    if (it != expr_results_.end()) {
        return it->second;
    }
    return "";
}

void IRGenerator::store_expr_result(Expr *node, const std::string &ir_var) {
    expr_results_[node] = ir_var;
}

void IRGenerator::begin_block(const std::string &label) {
    emitter_.begin_basic_block(label);
    current_block_label_ = label;
    current_block_terminated_ = false;
}

std::string IRGenerator::token_to_ir_op(const Token &op, bool is_unsigned) {
    switch (op.type) {
    case TokenType::PLUS:
        return "add";
    case TokenType::MINUS:
        return "sub";
    case TokenType::STAR:
        return "mul";
    case TokenType::SLASH:
        return is_unsigned ? "udiv" : "sdiv";
    case TokenType::PERCENT:
        return is_unsigned ? "urem" : "srem";
    case TokenType::AMPERSAND:
        return "and";
    case TokenType::PIPE:
        return "or";
    case TokenType::CARET:
        return "xor";
    case TokenType::LESS_LESS:
        return "shl";
    case TokenType::GREATER_GREATER:
        return is_unsigned ? "lshr" : "ashr";
    default:
        return "add";
    }
}

std::string IRGenerator::token_to_icmp_pred(const Token &op, bool is_unsigned) {
    switch (op.type) {
    case TokenType::EQUAL_EQUAL:
        return "eq";
    case TokenType::BANG_EQUAL:
        return "ne";
    case TokenType::LESS:
        return is_unsigned ? "ult" : "slt";
    case TokenType::LESS_EQUAL:
        return is_unsigned ? "ule" : "sle";
    case TokenType::GREATER:
        return is_unsigned ? "ugt" : "sgt";
    case TokenType::GREATER_EQUAL:
        return is_unsigned ? "uge" : "sge";
    default:
        return "eq";
    }
}

bool IRGenerator::is_signed_integer(Type *type) {
    if (!type)
        return false;

    auto prim_type = dynamic_cast<PrimitiveType *>(type);
    if (!prim_type)
        return false;

    return prim_type->kind == TypeKind::I32 || prim_type->kind == TypeKind::ISIZE;
}

int IRGenerator::get_integer_bits(TypeKind kind) {
    switch (kind) {
    case TypeKind::BOOL:
        return 1;
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
        return 32;
    default:
        return 32;
    }
}

bool IRGenerator::evaluate_const_expr(Expr *expr, std::string &result) {

    if (!expr) {
        return false;
    }

    if (auto literal = dynamic_cast<LiteralExpr *>(expr)) {
        if (literal->literal.type == TokenType::NUMBER) {
            result = literal->literal.lexeme;
            return true;
        } else if (literal->literal.type == TokenType::TRUE) {
            result = "1";
            return true;
        } else if (literal->literal.type == TokenType::FALSE) {
            result = "0";
            return true;
        }
        return false;
    }

    if (auto unary = dynamic_cast<UnaryExpr *>(expr)) {
        std::string operand_str;
        if (!evaluate_const_expr(unary->right.get(), operand_str)) {
            return false;
        }

        try {
            int operand = std::stoi(operand_str);
            int result_val = 0;

            if (unary->op.type == TokenType::MINUS) {
                result_val = -operand;
            } else if (unary->op.type == TokenType::BANG) {
                result_val = ~operand;
            } else {
                return false;
            }

            result = std::to_string(result_val);
            return true;
        } catch (...) {
            return false;
        }
    }

    if (auto binary = dynamic_cast<BinaryExpr *>(expr)) {
        std::string left_str, right_str;
        if (!evaluate_const_expr(binary->left.get(), left_str) ||
            !evaluate_const_expr(binary->right.get(), right_str)) {
            return false;
        }

        try {
            int left_val = std::stoi(left_str);
            int right_val = std::stoi(right_str);
            int result_val = 0;

            switch (binary->op.type) {
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
                if (right_val == 0)
                    return false;
                result_val = left_val / right_val;
                break;
            case TokenType::PERCENT:
                if (right_val == 0)
                    return false;
                result_val = left_val % right_val;
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
                return false;
            }

            result = std::to_string(result_val);
            return true;
        } catch (...) {
            return false;
        }
    }

    if (auto var_expr = dynamic_cast<VariableExpr *>(expr)) {
        std::string var_name = var_expr->name.lexeme;
        auto it = const_values_.find(var_name);
        if (it != const_values_.end()) {
            result = it->second;
            return true;
        }
        return false;
    }

    if (auto as_expr = dynamic_cast<AsExpr *>(expr)) {
        return evaluate_const_expr(as_expr->expression.get(), result);
    }

    if (auto grouping = dynamic_cast<GroupingExpr *>(expr)) {
        return evaluate_const_expr(grouping->expression.get(), result);
    }

    return false;
}

// Get the alignment requirement of a type.
size_t IRGenerator::get_type_alignment(Type *type) {
    if (!type)
        return 1;

    switch (type->kind) {
    case TypeKind::BOOL:
        return 1;
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::USIZE:
    case TypeKind::ISIZE:
        return 4;
    case TypeKind::REFERENCE:
        return 4;
    case TypeKind::ARRAY:
        if (auto arr_type = dynamic_cast<ArrayType *>(type)) {
            return get_type_alignment(arr_type->element_type.get());
        }
        return 1;
    case TypeKind::STRUCT:
        if (auto struct_type = dynamic_cast<StructType *>(type)) {
            size_t max_align = 1;
            for (const auto &field_name : struct_type->field_order) {
                auto it = struct_type->fields.find(field_name);
                if (it != struct_type->fields.end()) {
                    size_t field_align = get_type_alignment(it->second.get());
                    if (field_align > max_align) {
                        max_align = field_align;
                    }
                }
            }
            return max_align;
        }
        return 1;
    default:
        return 1;
    }
}

/**
 * Calculate the size in bytes of a type.
 *
 * Size table (32-bit platform):
 * - i32, u32, isize, usize, char: 4 bytes
 * - bool: 1 byte
 * - unit type (): 0 bytes
 * - pointers/references: 4 bytes
 * - arrays: element_size × array_length
 * - structs: sum of field sizes
 *
 * Features:
 * - Result caching to avoid redundant calculations
 * - Recursive size computation for nested types
 *
 * Limitations:
 * - Does not account for struct field padding/alignment
 * - Assumes tight packing of struct fields
 * - Platform-specific (32-bit assumed)
 *
 * @param type The type to measure
 * @return Size in bytes
 * @note Used for memcpy/memset size calculations
 */
size_t IRGenerator::get_type_size(Type *type) {
    if (!type)
        return 0;

    auto cache_it = type_size_cache_.find(type);
    if (cache_it != type_size_cache_.end()) {
        return cache_it->second;
    }

    size_t size = 0;

    switch (type->kind) {
    case TypeKind::BOOL:
        size = 1;
        break;
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::USIZE:
    case TypeKind::ISIZE:
        size = 4;
        break;
    case TypeKind::REFERENCE:
        size = 4;
        break;
    case TypeKind::ARRAY:
        if (auto arr_type = dynamic_cast<ArrayType *>(type)) {
            size_t elem_size = get_type_size(arr_type->element_type.get());
            size_t elem_align = get_type_alignment(arr_type->element_type.get());

            if (elem_size % elem_align != 0) {
                elem_size += (elem_align - (elem_size % elem_align));
            }

            size = elem_size * arr_type->size;
        }
        break;
    case TypeKind::STRUCT:
        if (auto struct_type = dynamic_cast<StructType *>(type)) {
            size_t offset = 0;
            size_t max_align = 1;

            for (const auto &field_name : struct_type->field_order) {
                auto it = struct_type->fields.find(field_name);
                if (it != struct_type->fields.end()) {
                    Type *field_type = it->second.get();
                    size_t field_size = get_type_size(field_type);
                    size_t field_align = get_type_alignment(field_type);

                    if (field_align > max_align) {
                        max_align = field_align;
                    }

                    if (offset % field_align != 0) {
                        offset += (field_align - (offset % field_align));
                    }

                    offset += field_size;
                }
            }

            if (offset % max_align != 0) {
                offset += (max_align - (offset % max_align));
            }

            size = offset;
        }
        break;
    default:
        size = 0;
        break;
    }

    type_size_cache_[type] = size;
    return size;
}

/**
 * Check if an expression is a zero initializer.
 *
 * Recursively checks:
 * - Integer literals with value 0
 * - Boolean literal false
 * - Array literals with all zero elements
 * - Struct initializers with all zero fields
 *
 * Purpose:
 * - Enables memset optimization for zero initialization
 * - Example: [0; 1000] can use single memset instead of 1000 stores
 * - Significantly reduces code size for large zero-initialized arrays
 *
 * @param expr The expression to check
 * @return true if expression evaluates to all zeros
 */
bool IRGenerator::is_zero_initializer(Expr *expr) {
    if (!expr)
        return false;

    if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
        if (lit->literal.type == TokenType::NUMBER) {
            try {
                long long val = std::stoll(lit->literal.lexeme);
                return val == 0;
            } catch (...) {
                return false;
            }
        }
        if (lit->literal.type == TokenType::TRUE || lit->literal.type == TokenType::FALSE) {
            return lit->literal.type == TokenType::FALSE;
        }
        return false;
    }

    if (auto struct_init = dynamic_cast<StructInitializerExpr *>(expr)) {
        for (const auto &field : struct_init->fields) {
            if (!is_zero_initializer(field->value.get())) {
                return false;
            }
        }
        return true;
    }

    if (auto array_lit = dynamic_cast<ArrayLiteralExpr *>(expr)) {
        for (const auto &elem : array_lit->elements) {
            if (!is_zero_initializer(elem.get())) {
                return false;
            }
        }
        return true;
    }

    return false;
}

bool IRGenerator::should_use_sret_optimization(const std::string &func_name, Type *return_type) {

    if (!return_type || return_type->kind != TypeKind::STRUCT)
        return false;

    size_t size = get_type_size(return_type);
    return size > 0;
}
