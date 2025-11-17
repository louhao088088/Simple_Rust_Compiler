/**
 * ir_generator_helpers.cpp
 *
 * IR生成器辅助函数
 *
 * 职责：
 * - 表达式结果存储和获取
 * - 基本块管理
 * - Token到IR运算符的转换
 * - 类型判断工具函数
 *
 * 这个文件包含IR生成过程中使用的各种辅助工具函数，
 * 不涉及具体的AST节点处理，而是提供通用的支持功能。
 */

#include "ir_generator.h"

// ========== 表达式结果管理 ==========

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

// ========== 基本块管理 ==========

void IRGenerator::begin_block(const std::string &label) {
    emitter_.begin_basic_block(label);
    current_block_label_ = label;
    current_block_terminated_ = false;
}

// ========== Token 转换函数 ==========

std::string IRGenerator::token_to_ir_op(const Token &op, bool is_unsigned) {
    // 将Token运算符转换为LLVM IR运算符
    // 用于算术和位运算
    switch (op.type) {
    case TokenType::PLUS:
        return "add";
    case TokenType::MINUS:
        return "sub";
    case TokenType::STAR:
        return "mul";
    case TokenType::SLASH:
        return is_unsigned ? "udiv" : "sdiv"; // 除法
    case TokenType::PERCENT:
        return is_unsigned ? "urem" : "srem"; // 取模
    case TokenType::AMPERSAND:
        return "and"; // 位与
    case TokenType::PIPE:
        return "or"; // 位或
    case TokenType::CARET:
        return "xor"; // 位异或
    case TokenType::LESS_LESS:
        return "shl"; // 左移
    case TokenType::GREATER_GREATER:
        return is_unsigned ? "lshr" : "ashr"; // 右移
    default:
        return "add";
    }
}

std::string IRGenerator::token_to_icmp_pred(const Token &op, bool is_unsigned) {
    // 将Token比较运算符转换为LLVM icmp谓词
    // icmp指令用于整数比较
    switch (op.type) {
    case TokenType::EQUAL_EQUAL:
        return "eq"; // equal
    case TokenType::BANG_EQUAL:
        return "ne"; // not equal
    case TokenType::LESS:
        return is_unsigned ? "ult" : "slt"; // unsigned/signed less than
    case TokenType::LESS_EQUAL:
        return is_unsigned ? "ule" : "sle"; // unsigned/signed less or equal
    case TokenType::GREATER:
        return is_unsigned ? "ugt" : "sgt"; // unsigned/signed greater than
    case TokenType::GREATER_EQUAL:
        return is_unsigned ? "uge" : "sge"; // unsigned/signed greater or equal
    default:
        return "eq";
    }
}

// ========== 类型判断函数 ==========

bool IRGenerator::is_signed_integer(Type *type) {
    // 判断类型是否为有符号整数（i32或isize）
    // 用于选择正确的扩展指令（sext vs zext）
    if (!type)
        return false;

    auto prim_type = dynamic_cast<PrimitiveType *>(type);
    if (!prim_type)
        return false;

    return prim_type->kind == TypeKind::I32 || prim_type->kind == TypeKind::ISIZE;
}

int IRGenerator::get_integer_bits(TypeKind kind) {
    // 获取整数类型的位宽
    // 用于类型转换时选择正确的指令
    switch (kind) {
    case TypeKind::BOOL:
        return 1; // i1
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
        // 32位平台：所有标准整数类型都是32位
        return 32;
    default:
        return 32; // 默认32位
    }
}

// ========== 常量表达式求值 ==========

bool IRGenerator::evaluate_const_expr(Expr *expr, std::string &result) {
    // 编译时求值常量表达式
    // 支持：字面量、一元运算、二元运算、常量引用

    if (!expr) {
        return false;
    }

    // 1. 字面量
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

    // 2. 一元运算符（-, !）
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
                result_val = ~operand; // 按位取反
            } else {
                return false;
            }

            result = std::to_string(result_val);
            return true;
        } catch (...) {
            return false;
        }
    }

    // 3. 二元运算符（+, -, *, /, %, &, |, ^, <<, >>）
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

    // 4. 变量引用（查找常量表）
    if (auto var_expr = dynamic_cast<VariableExpr *>(expr)) {
        std::string var_name = var_expr->name.lexeme;
        auto it = const_values_.find(var_name);
        if (it != const_values_.end()) {
            result = it->second;
            return true;
        }
        return false;
    }

    // 5. As表达式（类型转换）
    if (auto as_expr = dynamic_cast<AsExpr *>(expr)) {
        // 对于常量表达式，as只是类型标注，值不变
        return evaluate_const_expr(as_expr->expression.get(), result);
    }

    // 6. 分组表达式（括号）
    if (auto grouping = dynamic_cast<GroupingExpr *>(expr)) {
        return evaluate_const_expr(grouping->expression.get(), result);
    }

    return false;
}
// ========== 类型大小计算 ==========

size_t IRGenerator::get_type_size(Type *type) {
    if (!type)
        return 0;

    switch (type->kind) {
    case TypeKind::BOOL:
        return 1; // i1 -> 1 byte
    case TypeKind::I32:
    case TypeKind::U32:
        return 4;
    case TypeKind::USIZE:
    case TypeKind::ISIZE:
    case TypeKind::REFERENCE:
        return 8; // pointer size on x64
    case TypeKind::ARRAY:
        if (auto arr_type = dynamic_cast<ArrayType *>(type)) {
            size_t elem_size = get_type_size(arr_type->element_type.get());
            return elem_size * arr_type->size;
        }
        return 0;
    case TypeKind::STRUCT:
        if (auto struct_type = dynamic_cast<StructType *>(type)) {
            // 计算结构体大小（简单求和，不考虑对齐）
            size_t total = 0;
            for (const auto &field_name : struct_type->field_order) {
                auto it = struct_type->fields.find(field_name);
                if (it != struct_type->fields.end()) {
                    total += get_type_size(it->second.get());
                }
            }
            return total;
        }
        return 0;
    default:
        return 0;
    }
}

// ========== 初始化检测 ==========

bool IRGenerator::is_zero_initializer(Expr *expr) {
    if (!expr)
        return false;

    // 1. 检查整数字面量是否为0
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

    // 2. 检查结构体初始化器是否所有字段都为0
    if (auto struct_init = dynamic_cast<StructInitializerExpr *>(expr)) {
        for (const auto &field : struct_init->fields) {
            if (!is_zero_initializer(field->value.get())) {
                return false;
            }
        }
        return true;
    }

    // 3. 检查数组字面量是否所有元素都为0
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
    // 1. 检查函数名是否以 _new 结尾
    if (func_name.length() < 4)
        return false;
    if (func_name.substr(func_name.length() - 4) != "_new")
        return false;

    // 2. 检查返回类型是否为结构体
    if (!return_type || return_type->kind != TypeKind::STRUCT)
        return false;

    // 3. 检查结构体大小是否 > 64 字节
    size_t size = get_type_size(return_type);
    return size > 64;
}
