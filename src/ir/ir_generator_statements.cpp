/**
 * @file ir_generator_statements.cpp
 * @brief IR生成器 - 语句处理模块
 *
 * 本文件包含所有Statement节点的IR生成实现：
 * - BlockStmt: 块语句（作用域管理）
 * - ExprStmt: 表达式语句
 * - LetStmt: 变量声明语句（包括聚合类型处理）
 * - ReturnStmt: 返回语句
 * - BreakStmt/ContinueStmt: 循环控制语句
 * - ItemStmt: 项语句
 */

#include "ir_generator.h"

#include <map>
#include <set>

// ========== Statement Visitors ==========

void IRGenerator::visit(BlockStmt *node) {
    // 进入新作用域
    value_manager_.enter_scope();

    // 处理所有语句，跳过被后续同名变量声明shadow的无用声明
    for (size_t i = 0; i < node->statements.size(); ++i) {
        // 如果当前基本块已终止（有return/break/continue），跳过后续语句
        if (current_block_terminated_) {
            break;
        }

        auto &stmt = node->statements[i];

        // 优化：检测连续的同名变量声明（shadowing 死代码消除）
        // 如果当前是 LetStmt，且下一个语句也是同名变量的 LetStmt，跳过当前语句
        bool should_skip = false;
        if (auto let_stmt = dynamic_cast<LetStmt *>(stmt.get())) {
            if (auto id_pattern = dynamic_cast<IdentifierPattern *>(let_stmt->pattern.get())) {
                std::string var_name = id_pattern->name.lexeme;

                // 检查下一个语句
                if (i + 1 < node->statements.size()) {
                    if (auto next_let = dynamic_cast<LetStmt *>(node->statements[i + 1].get())) {
                        if (auto next_pattern =
                                dynamic_cast<IdentifierPattern *>(next_let->pattern.get())) {
                            if (next_pattern->name.lexeme == var_name) {
                                // 连续的同名变量声明，跳过当前声明
                                should_skip = true;
                            }
                        }
                    }
                }
            }
        }

        if (!should_skip) {
            stmt->accept(this);
        }
    }

    // 处理最终表达式（如果有）
    if (node->final_expr.has_value()) {
        auto final_expr = node->final_expr.value();
        if (final_expr) {
            // 标志已经在 visit_function_decl 中设置，直接生成
            final_expr->accept(this);
            // 如果块有返回值，结果已经存储在 expr_results_ 中
            // BlockExpr 会读取它
        }
    }

    // 退出作用域
    value_manager_.exit_scope();
}

void IRGenerator::visit(ExprStmt *node) {
    // 计算表达式（可能有副作用）
    if (node->expression) {
        node->expression->accept(this);
    }
}

void IRGenerator::visit(LetStmt *node) {
    // let x: i32 = 10;
    // let mut y = 20;

    // 1. 获取变量名和可变性
    auto id_pattern = dynamic_cast<IdentifierPattern *>(node->pattern.get());
    if (!id_pattern) {
        // TODO: 处理其他模式（元组解构等）
        return;
    }

    std::string var_name = id_pattern->name.lexeme;
    bool is_mutable = id_pattern->is_mutable;

    // 2. 获取类型
    std::shared_ptr<Type> var_type;

    if (node->type_annotation.has_value() && node->type_annotation.value()->resolved_type) {
        // 有类型注解
        var_type = node->type_annotation.value()->resolved_type;
    } else if (node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        if (init_expr && init_expr->type) {
            // 从初始化表达式推导类型
            var_type = init_expr->type;
        }
    }

    if (!var_type) {
        // 没有类型信息
        return;
    }

    std::string type_str = type_mapper_.map(var_type.get());

    // 3. 检查是否是引用类型
    bool is_reference = (var_type->kind == TypeKind::REFERENCE);

    // 4. 对于数组和结构体字面量，以及返回聚合类型的函数调用，
    //    表达式本身已经分配并初始化了内存，我们只需要使用它返回的指针
    bool is_aggregate_returns_pointer = false;
    std::string alloca_name;

    if (node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        if (init_expr) {
            // 检查是否是数组字面量、数组初始化器或结构体初始化器
            bool is_literal = (dynamic_cast<ArrayLiteralExpr *>(init_expr.get()) ||
                               dynamic_cast<ArrayInitializerExpr *>(init_expr.get()) ||
                               dynamic_cast<StructInitializerExpr *>(init_expr.get()));

            // 检查是否是返回聚合类型的函数调用
            bool is_call_ret_aggregate = false;
            if (auto call_expr = dynamic_cast<CallExpr *>(init_expr.get())) {
                if (call_expr->type) {
                    is_call_ret_aggregate = (call_expr->type->kind == TypeKind::ARRAY ||
                                             call_expr->type->kind == TypeKind::STRUCT);
                }
            }

            is_aggregate_returns_pointer = (is_literal || is_call_ret_aggregate);
        }
    }

    // 5. 引用类型直接使用初始化表达式返回的指针，不创建alloca
    if (is_reference && node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        init_expr->accept(this);
        alloca_name = get_expr_result(init_expr.get());

        // 引用类型已经是指针，直接注册（type_str已经是T*形式）
        value_manager_.define_variable(var_name, alloca_name, type_str, is_mutable);
        return;
    }

    if (is_aggregate_returns_pointer) {
        // 表达式已经返回指针，直接使用
        auto init_expr = node->initializer.value();
        init_expr->accept(this);
        alloca_name = get_expr_result(init_expr.get());
    } else {
        // 普通情况：分配栈空间，然后可能初始化
        alloca_name = emitter_.emit_alloca(type_str);

        // 4. 如果有初始化表达式，计算并存储
        if (node->initializer.has_value()) {
            auto init_expr = node->initializer.value();
            if (init_expr) {
                // 优化：对于聚合类型，尝试使用目标地址传递（原地初始化）
                // 这可以避免 StructInitializer/ArrayInitializer 创建临时 alloca
                bool is_aggregate =
                    (var_type->kind == TypeKind::ARRAY || var_type->kind == TypeKind::STRUCT);

                if (is_aggregate) {
                    set_target_address(alloca_name);
                }

                init_expr->accept(this);

                // 清除未使用的 target（如果子表达式没有使用它）
                take_target_address();

                std::string init_value = get_expr_result(init_expr.get());
                if (!init_value.empty()) {
                    // 如果发生了原地初始化，init_value 应该是 alloca_name
                    // 此时不需要 store/memcpy
                    if (is_aggregate && init_value == alloca_name) {
                        // 已经原地初始化完成
                    } else if (is_aggregate) {
                        // 聚合类型：使用 memcpy 优化
                        size_t size = get_type_size(var_type.get());
                        std::string ptr_type = type_str + "*";
                        emitter_.emit_memcpy(alloca_name, init_value, size, ptr_type);
                    } else {
                        emitter_.emit_store(type_str, init_value, alloca_name);
                    }
                }
            }
        }
    }

    // 5. 注册到 ValueManager（存储指针类型）
    std::string ptr_type_str = type_str + "*";
    value_manager_.define_variable(var_name, alloca_name, ptr_type_str, is_mutable);
}

void IRGenerator::visit(ReturnStmt *node) {
    if (node->value.has_value()) {
        // return expr;
        auto return_expr = node->value.value();
        if (return_expr) {
            return_expr->accept(this);

            std::string return_value = get_expr_result(return_expr.get());

            if (return_expr->type) {
                std::string expr_type_str = type_mapper_.map(return_expr->type.get());

                // 如果当前函数使用 sret，直接 ret void（结构体已在 self 中）
                if (current_function_uses_sret_) {
                    emitter_.emit_ret_void();
                } else {
                    // 检查是否为聚合类型返回值
                    bool is_aggregate = (return_expr->type->kind == TypeKind::ARRAY ||
                                         return_expr->type->kind == TypeKind::STRUCT);

                    if (is_aggregate) {
                        // 聚合类型：需要load值（因为return_value是指针）
                        return_value = emitter_.emit_load(expr_type_str, return_value);
                    }

                    // 检查返回值类型是否与函数返回类型匹配
                    // 如果不匹配，需要插入类型转换指令
                    if (!current_function_return_type_str_.empty() &&
                        expr_type_str != current_function_return_type_str_) {
                        // 需要类型转换
                        // 目前仅处理整数类型的扩展/截断
                        if ((expr_type_str == "i32" || expr_type_str == "i64") &&
                            (current_function_return_type_str_ == "i32" ||
                             current_function_return_type_str_ == "i64")) {
                            // 整数类型转换
                            if (expr_type_str == "i32" &&
                                current_function_return_type_str_ == "i64") {
                                // i32 -> i64: 符号扩展
                                return_value = emitter_.emit_sext(return_value, "i32", "i64");
                            } else if (expr_type_str == "i64" &&
                                       current_function_return_type_str_ == "i32") {
                                // i64 -> i32: 截断
                                return_value = emitter_.emit_trunc(return_value, "i64", "i32");
                            }
                        }
                    }

                    // 使用函数返回类型（如果可用）或表达式类型
                    std::string ret_type = current_function_return_type_str_.empty()
                                               ? expr_type_str
                                               : current_function_return_type_str_;
                    emitter_.emit_ret(ret_type, return_value);
                }
                current_block_terminated_ = true; // 标记基本块已终止
            }
        }
    } else {
        // return;
        emitter_.emit_ret_void();
        current_block_terminated_ = true; // 标记基本块已终止
    }
}

void IRGenerator::visit(BreakStmt *node) {
    // break 语句：跳转到最内层循环的结束标签
    if (loop_stack_.empty()) {
        // 错误：不在循环中使用 break（应该在语义分析阶段检查）
        // 这里简单忽略
        return;
    }

    // 获取最内层循环的 break 目标
    std::string break_label = loop_stack_.back().break_label;

    // 如果 break 有值（break value），需要处理返回值
    // 但目前我们先实现简单版本，假设 break 不带值
    if (node->value.has_value()) {
        // TODO: 处理 break value
        // 这需要循环表达式支持返回值和 phi 节点
    }

    // 生成无条件跳转到循环结束
    emitter_.emit_br(break_label);
    current_block_terminated_ = true; // 标记基本块已终止
}

void IRGenerator::visit(ContinueStmt *node) {
    // continue 语句：跳转到最内层循环的继续标签
    if (loop_stack_.empty()) {
        // 错误：不在循环中使用 continue（应该在语义分析阶段检查）
        // 这里简单忽略
        return;
    }

    // 获取最内层循环的 continue 目标
    std::string continue_label = loop_stack_.back().continue_label;

    // 生成无条件跳转到循环继续位置
    emitter_.emit_br(continue_label);
    current_block_terminated_ = true; // 标记基本块已终止
}

void IRGenerator::visit(ItemStmt *node) {
    // ItemStmt包装了一个Item（例如函数内的const声明、嵌套函数或local struct）

    if (!node->item) {
        return;
    }

    // 处理嵌套函数定义
    if (auto fn_decl = dynamic_cast<FnDecl *>(node->item.get())) {
        if (inside_function_body_) {
            // 在函数体内定义的函数：收集起来稍后提升为顶层函数
            nested_functions_.push_back(fn_decl);
        } else {
            // 顶层函数：直接处理
            visit_function_decl(fn_decl);
        }
        return;
    }

    // 处理local struct定义
    if (auto struct_decl = dynamic_cast<StructDecl *>(node->item.get())) {
        // Local struct已经在collect_all_structs阶段处理并生成
        // 这里不需要重复生成
        return;
    }

    // 对于局部const声明，将其视为不可变的局部变量
    if (auto const_decl = dynamic_cast<ConstDecl *>(node->item.get())) {
        // 获取类型
        if (!const_decl->type || !const_decl->type->resolved_type) {
            return;
        }

        std::string llvm_type = type_mapper_.map(const_decl->type->resolved_type.get());
        std::string const_name = const_decl->name.lexeme;

        // 为const创建alloca（虽然是常量，但在LLVM IR中仍需要存储位置）
        std::string alloca_ptr = emitter_.emit_alloca(llvm_type);

        // 计算初始值
        const_decl->value->accept(this);
        std::string init_value = get_expr_result(const_decl->value.get());

        if (init_value.empty()) {
            return;
        }

        // 存储初始值
        emitter_.emit_store(llvm_type, init_value, alloca_ptr);

        // 注册到ValueManager（标记为不可变）
        value_manager_.define_variable(const_name, alloca_ptr, llvm_type + "*", false);
    }
    // 可以扩展处理其他类型的局部Item（如果需要）
}
