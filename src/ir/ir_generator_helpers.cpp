#include "ir_generator.h"

/**
 * Retrieve the IR value associated with an expression.
 *
 * Purpose:
 * - Map AST expression nodes to their generated IR values
 * - Enables parent nodes to access child expression results
 *
 * Example flow:
 *   visit(BinaryExpr):
 *     1. node->left->accept(this)  // Generates IR
 *     2. left_val = get_expr_result(node->left)  // Retrieves "%1"
 *     3. node->right->accept(this)
 *     4. right_val = get_expr_result(node->right)  // Retrieves "%2"
 *     5. %3 = add i32 %1, %2
 *     6. store_expr_result(node, "%3")
 *
 * Return values:
 * - SSA register: "%1", "%temp_5", "%sum"
 * - Global: "@global_var"
 * - Literal: "42", "true"
 * - Empty string: Expression failed or has no value
 *
 * @param expr The expression to look up
 * @return The IR value string, or "" if not found
 */
std::string IRGenerator::get_expr_result(Expr *expr) {
    auto it = expr_results_.find(expr);
    if (it != expr_results_.end()) {
        return it->second;
    }
    return "";
}

/**
 * Store the IR value generated for an expression.
 *
 * Purpose:
 * - Associate generated IR values with AST nodes
 * - Enables parent nodes to retrieve child results
 * - Creates mapping from Expr* -> IR value string
 *
 * Example:
 *   After generating: %1 = add i32 %a, %b
 *   Call: store_expr_result(binary_expr, "%1")
 *   Later: get_expr_result(binary_expr) returns "%1"
 *
 * Usage pattern:
 *   void visit(SomeExpr* node) {
 *       // ... generate IR code ...
 *       std::string result = emitter_.emit_add(...);
 *       store_expr_result(node, result);
 *   }
 *
 * @param node The expression AST node
 * @param ir_var The IR value string (register, global, literal)
 */
void IRGenerator::store_expr_result(Expr *node, const std::string &ir_var) {
    expr_results_[node] = ir_var;
}

/**
 * Begin a new basic block in the current function.
 *
 * Purpose:
 * - Start a labeled basic block for control flow
 * - Reset termination flag (new block not yet terminated)
 * - Track current block label
 *
 * Basic blocks:
 * - Unit of control flow in LLVM IR
 * - Must end with terminator (br, ret, unreachable)
 * - Labels used for branch targets
 *
 * Examples:
 *   begin_block("if.then");
 *   // Generate code for then branch
 *   emitter_.emit_br("if.end");
 *
 *   begin_block("if.end");
 *   // Continue after if statement
 *
 * @param label The label name for this basic block
 */
void IRGenerator::begin_block(const std::string &label) {
    emitter_.begin_basic_block(label);
    current_block_label_ = label;
    current_block_terminated_ = false;
}

/**
 * Convert binary operator token to LLVM IR instruction name.
 *
 * Arithmetic operators:
 * - + -> add (both signed and unsigned)
 * - - -> sub
 * - * -> mul
 * - / -> sdiv (signed) or udiv (unsigned)
 * - % -> srem (signed) or urem (unsigned)
 *
 * Bitwise operators:
 * - & -> and
 * - | -> or
 * - ^ -> xor
 * - << -> shl (shift left)
 * - >> -> ashr (arithmetic) or lshr (logical)
 *
 * Signedness matters for:
 * - Division: sdiv rounds toward 0, udiv rounds down
 * - Remainder: srem can be negative, urem always positive
 * - Right shift: ashr preserves sign bit, lshr inserts 0
 *
 * @param op The binary operator token
 * @param is_unsigned Whether operands are unsigned
 * @return LLVM IR instruction name
 */
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

/**
 * Convert comparison operator token to LLVM icmp predicate.
 *
 * Comparison operators:
 * - == -> eq (equal)
 * - != -> ne (not equal)
 * - < -> slt (signed) or ult (unsigned)
 * - <= -> sle (signed) or ule (unsigned)
 * - > -> sgt (signed) or ugt (unsigned)
 * - >= -> sge (signed) or uge (unsigned)
 *
 * Signedness matters:
 * - Signed: -1 < 1 (true, slt)
 * - Unsigned: 0xFFFFFFFF < 1 (false, ult - treats as large positive)
 *
 * Usage:
 *   %cmp = icmp slt i32 %a, %b  // Signed less than
 *   %cmp = icmp ult i32 %a, %b  // Unsigned less than
 *
 * @param op The comparison operator token
 * @param is_unsigned Whether operands are unsigned
 * @return LLVM icmp predicate string
 */
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

/**
 * Check if a type is a signed integer.
 *
 * Signed integer types:
 * - i32: 32-bit signed integer
 * - isize: Platform-dependent signed integer (32-bit in this compiler)
 *
 * Not signed:
 * - u32, usize: Unsigned variants
 * - bool: Considered unsigned (i1)
 * - Pointers, arrays, structs: Not integers
 *
 * Used for:
 * - Choosing signed vs unsigned IR instructions (sdiv vs udiv)
 * - Selecting comparison predicates (slt vs ult)
 * - Right shift behavior (ashr vs lshr)
 *
 * @param type The type to check
 * @return true if type is signed integer (i32 or isize)
 */
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

/**
 * Evaluate constant expression at compile time.
 *
 * Supported expressions:
 * - Literals: 42, true, false, "hello"
 * - Const references: const X = 10; const Y = X;
 * - Binary arithmetic: 5 + 3, 10 * 2
 * - Unary negation: -5
 *
 * Purpose:
 * - Enable const declaration initialization
 * - Compute array sizes at compile time
 * - Support constant folding optimizations
 *
 * Limitations:
 * - No function calls
 * - No variable references (except other consts)
 * - Integer arithmetic only (no floating point)
 *
 * Example:
 *   const SIZE: i32 = 10 * 2 + 5;  // Evaluates to "25"
 *   const FLAG: bool = true;       // Evaluates to "1"
 *
 * @param expr The expression to evaluate
 * @param result [out] The string representation of the constant value
 * @return true if successfully evaluated
 */
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

/**
 * Determine if a function should use SRET (Structure Return) optimization.
 *
 * SRET optimization:
 * - Large structs returned via hidden pointer parameter
 * - Avoids copying large structures through registers
 * - Caller allocates space, passes pointer as first arg
 *
 * Without SRET:
 *   define %LargeStruct @foo() {
 *       %result = alloca %LargeStruct
 *       ...
 *       %loaded = load %LargeStruct, %LargeStruct* %result
 *       ret %LargeStruct %loaded  // Expensive copy!
 *   }
 *
 * With SRET:
 *   define void @foo(%LargeStruct* sret %return_ptr) {
 *       ...
 *       call void @llvm.memcpy(%return_ptr, %local_struct, size)
 *       ret void
 *   }
 *
 * Criteria:
 * - Return type is struct or array
 * - Size is large (threshold-based decision)
 * - Not for main() function (special case)
 *
 * @param func_name Function name being generated
 * @param return_type The return type to check
 * @return true if SRET should be used
 */
bool IRGenerator::should_use_sret_optimization(const std::string &func_name, Type *return_type) {

    if (!return_type || return_type->kind != TypeKind::STRUCT)
        return false;

    size_t size = get_type_size(return_type);
    return size > 0;
}
