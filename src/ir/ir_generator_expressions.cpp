/**
 * @file ir_generator_expressions.cpp
 * @brief IR生成器 - 基础表达式处理模块
 *
 * 本文件包含基础Expression节点的IR生成实现：
 * - LiteralExpr: 字面量（数字、布尔、字符）
 * - VariableExpr: 变量引用（包括const常量）
 * - BinaryExpr: 二元运算（算术、比较、逻辑）
 * - UnaryExpr: 一元运算（负号、逻辑非、解引用）
 * - CallExpr: 函数调用（包括方法调用）
 * - AssignmentExpr: 赋值表达式
 * - GroupingExpr: 括号表达式
 * - BlockExpr: 块表达式
 * - AsExpr: 类型转换
 * - ReferenceExpr: 引用表达式
 * - 其他未实现的表达式
 */

#include "ir_generator.h"

// ========== Expression Visitors ==========

void IRGenerator::visit(LiteralExpr *node) {
    if (!node->type) {
        return;
    }

    std::string type_str = type_mapper_.map(node->type.get());
    std::string value;

    switch (node->literal.type) {
    case TokenType::NUMBER: {
        // 整数字面量 - 支持十进制、十六进制(0x)、八进制(0o)、二进制(0b)
        std::string lexeme = node->literal.lexeme;

        // 移除类型后缀（如 "42i32" -> "42"）
        size_t suffix_pos = lexeme.find_first_of("iu");
        if (suffix_pos != std::string::npos) {
            lexeme = lexeme.substr(0, suffix_pos);
        }

        // 解析不同进制
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

        // 转换为十进制字符串
        try {
            long long num_value = std::stoll(lexeme.substr(start_pos), nullptr, base);
            value = std::to_string(num_value);
        } catch (...) {
            // 解析失败，回退到0
            value = "0";
        }
        break;
    }
    case TokenType::TRUE:
        value = "1"; // bool true -> i1 1
        break;
    case TokenType::FALSE:
        value = "0"; // bool false -> i1 0
        break;
    case TokenType::CHAR: {
        // 字符字面量 -> i8
        if (node->literal.lexeme.length() >= 3) {
            char c = node->literal.lexeme[1]; // 'a' -> a
            value = std::to_string(static_cast<int>(static_cast<unsigned char>(c)));
        }
        break;
    }
    case TokenType::STRING:
        // TODO: 字符串字面量处理（需要全局常量）
        value = "null"; // 暂时返回 null
        break;
    default:
        value = "0";
        break;
    }

    // 字面量直接是常量，不需要 load
    store_expr_result(node, value);
}

void IRGenerator::visit(VariableExpr *node) {
    std::string var_name = node->name.lexeme;

    // 首先查找局部变量（包括局部const）
    VariableInfo *var_info = value_manager_.lookup_variable(var_name);

    if (var_info) {
        // 找到局部变量
        if (!node->type) {
            store_expr_result(node, "");
            return;
        }

        // 对于数组和结构体类型，返回指针而不是加载值
        // 因为这些类型在LLVM IR中按引用传递
        // 引用类型参数（&T）也应该直接返回指针，不需要load
        // 如果generating_lvalue_为true（例如在取地址&操作中），也应该返回指针
        bool is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
        bool is_reference = (node->type->kind == TypeKind::REFERENCE);

        if (is_aggregate || is_reference || generating_lvalue_) {
            // 数组/结构体/引用/左值：直接返回指针
            store_expr_result(node, var_info->alloca_name);
        } else {
            // 普通类型右值：加载值
            std::string type_str = type_mapper_.map(node->type.get());
            std::string loaded_value = emitter_.emit_load(type_str, var_info->alloca_name);
            store_expr_result(node, loaded_value);
        }
        return;
    }

    // 如果不是局部变量，检查是否是全局const常量
    if (node->resolved_symbol && node->resolved_symbol->kind == Symbol::CONSTANT) {
        // 全局const常量：从全局常量加载
        std::string const_name = node->name.lexeme;
        std::string type_str = type_mapper_.map(node->type.get());

        // 加载全局常量: %0 = load i32, i32* @MAX
        std::string loaded_value = emitter_.emit_load(type_str, "@" + const_name);
        store_expr_result(node, loaded_value);
        return;
    }

    // 变量未找到
    store_expr_result(node, "");
}

void IRGenerator::visit(BinaryExpr *node) {
    if (!node->type) {
        return;
    }

    // 特殊处理：短路求值的逻辑运算符
    if (node->op.type == TokenType::AMPERSAND_AMPERSAND || node->op.type == TokenType::PIPE_PIPE) {
        visit_logical_binary_expr(node);
        return;
    }

    // 1. 计算左右操作数
    node->left->accept(this);
    node->right->accept(this);

    std::string left_var = get_expr_result(node->left.get());
    std::string right_var = get_expr_result(node->right.get());

    if (left_var.empty() || right_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 2. 获取操作数类型
    std::string type_str = type_mapper_.map(node->type.get());

    // 判断操作数是否为无符号类型
    bool is_unsigned = false;
    if (node->left->type) {
        TypeKind kind = node->left->type->kind;
        is_unsigned = (kind == TypeKind::U32 || kind == TypeKind::USIZE);
    }

    // 3. 根据运算符生成指令
    std::string result;

    if (node->op.type == TokenType::EQUAL_EQUAL || node->op.type == TokenType::BANG_EQUAL ||
        node->op.type == TokenType::LESS || node->op.type == TokenType::LESS_EQUAL ||
        node->op.type == TokenType::GREATER || node->op.type == TokenType::GREATER_EQUAL) {
        // 比较运算 -> icmp
        std::string pred = token_to_icmp_pred(node->op, is_unsigned);

        // 获取操作数类型（不是结果类型）
        std::string operand_type_str;
        if (node->left->type) {
            operand_type_str = type_mapper_.map(node->left->type.get());
        } else {
            operand_type_str = "i32"; // 默认
        }

        result = emitter_.emit_icmp(pred, operand_type_str, left_var, right_var);
    } else {
        // 算术运算
        std::string ir_op = token_to_ir_op(node->op, is_unsigned);
        result = emitter_.emit_binary_op(ir_op, type_str, left_var, right_var);
    }

    // 4. 存储结果
    store_expr_result(node, result);
}

void IRGenerator::visit(UnaryExpr *node) {
    if (!node->type) {
        return;
    }

    // 计算操作数
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
        // -x -> 0 - x
        result = emitter_.emit_neg(type_str, operand);
        break;
    case TokenType::BANG:
        // !x
        if (type_str == "i1") {
            // bool类型：逻辑非
            result = emitter_.emit_not(operand);
        } else {
            // 整数类型：位取反 (xor with -1)
            result = emitter_.emit_binary_op("xor", type_str, operand, "-1");
        }
        break;
    case TokenType::STAR:
        // *x (解引用)
        // 对于引用类型，解引用得到被引用的类型
        // 如果被引用的类型是聚合类型（数组/结构体），不应该load值，而是返回指针
        if (node->type) {
            bool is_aggregate =
                (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
            if (is_aggregate) {
                // 聚合类型：解引用返回指针（与VariableExpr的聚合类型处理一致）
                result = operand;
            } else {
                // 非聚合类型：解引用load值
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

// 逻辑运算符短路求值实现
void IRGenerator::visit_logical_binary_expr(BinaryExpr *node) {
    bool is_or = (node->op.type == TokenType::PIPE_PIPE);

    // 生成唯一的标签
    static int logical_counter = 0;
    int current = logical_counter++;

    std::string rhs_label = (is_or ? "or.rhs." : "and.rhs.") + std::to_string(current);
    std::string end_label = (is_or ? "or.end." : "and.end.") + std::to_string(current);

    // 1. 计算左操作数
    node->left->accept(this);
    std::string left_var = get_expr_result(node->left.get());

    if (left_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 记录左侧计算完成时的块（用于PHI）
    std::string left_block = current_block_label_;

    // 2. 根据左操作数的值决定是否需要计算右操作数
    if (is_or) {
        // a || b: 如果a为真，跳到end；否则计算b
        emitter_.emit_cond_br(left_var, end_label, rhs_label);
    } else {
        // a && b: 如果a为假，跳到end；否则计算b
        emitter_.emit_cond_br(left_var, rhs_label, end_label);
    }

    // 3. 右操作数块
    begin_block(rhs_label);
    current_block_terminated_ = false;

    node->right->accept(this);
    std::string right_var = get_expr_result(node->right.get());

    if (right_var.empty()) {
        // 右侧计算失败，整个表达式失败
        // 需要生成一个合法的控制流结束
        // 为了保持控制流完整性，使用左侧的值作为结果
        emitter_.emit_br(end_label);
        begin_block(end_label);
        current_block_terminated_ = false;
        store_expr_result(node, left_var);
        return;
    }

    // 记录右侧计算完成时的块（必须在emit_br之前）
    std::string right_block = current_block_label_;

    // 跳转到end（只在块未终止时）
    if (!current_block_terminated_) {
        emitter_.emit_br(end_label);
    }

    // 4. 结束块：使用PHI合并结果
    begin_block(end_label);
    current_block_terminated_ = false;

    // PHI节点：从左块来的是left_var，从右块来的是right_var
    std::vector<std::pair<std::string, std::string>> phi_incoming;
    phi_incoming.push_back({left_var, left_block});
    phi_incoming.push_back({right_var, right_block});

    std::string result = emitter_.emit_phi("i1", phi_incoming);
    store_expr_result(node, result);
}

void IRGenerator::visit(CallExpr *node) {
    // 1. 计算所有参数
    std::vector<std::pair<std::string, std::string>> args; // (type, value)

    for (const auto &arg : node->arguments) {
        arg->accept(this);
        std::string arg_value = get_expr_result(arg.get());

        if (arg_value.empty()) {
            // 如果参数计算失败，跳过
            continue;
        }

        if (!arg->type) {
            // 如果没有类型信息，可能是语义分析阶段的问题
            // 暂时跳过或使用默认处理
            continue;
        }

        std::string arg_type_str = type_mapper_.map(arg->type.get());

        // 检查是否为聚合类型或引用类型
        bool is_aggregate =
            (arg->type->kind == TypeKind::ARRAY || arg->type->kind == TypeKind::STRUCT);
        bool is_reference = (arg->type->kind == TypeKind::REFERENCE);

        if (is_aggregate || is_reference) {
            // 聚合类型或引用类型：传递指针
            // arg_value 应该已经是指针
            if (is_reference) {
                // 引用类型：需要解引用获取实际类型
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
            // 基础类型：直接传值
            args.push_back({arg_type_str, arg_value});
        }
    }

    // 2. 获取函数名
    std::string func_name;
    std::vector<std::pair<std::string, std::string>> self_args; // self参数（如果是实例方法）

    if (auto var_expr = dynamic_cast<VariableExpr *>(node->callee.get())) {
        func_name = var_expr->name.lexeme;
    } else if (auto path_expr = dynamic_cast<PathExpr *>(node->callee.get())) {
        // 处理 Type::method 形式的调用（关联函数）
        // 例如: Point::new(...) -> Point_new(...)

        // 获取类型名和方法名
        std::string type_name;
        std::string method_name;

        if (auto left_var = dynamic_cast<VariableExpr *>(path_expr->left.get())) {
            type_name = left_var->name.lexeme;
        }

        if (auto right_var = dynamic_cast<VariableExpr *>(path_expr->right.get())) {
            method_name = right_var->name.lexeme;
        }

        if (!type_name.empty() && !method_name.empty()) {
            // 应用name mangling
            func_name = type_name + "_" + method_name;
        } else {
            store_expr_result(node, "");
            return;
        }
    } else if (auto field_expr = dynamic_cast<FieldAccessExpr *>(node->callee.get())) {
        // 处理 obj.method() 形式的调用（实例方法）
        // 例如: p.distance_squared() -> Point_distance_squared(&p)

        // 计算对象表达式（得到self）
        field_expr->object->accept(this);
        std::string obj_ptr = get_expr_result(field_expr->object.get());

        if (obj_ptr.empty()) {
            store_expr_result(node, "");
            return;
        }

        // 获取对象类型和方法名
        std::string type_name;
        if (field_expr->object->type) {
            if (auto struct_type =
                    std::dynamic_pointer_cast<StructType>(field_expr->object->type)) {
                type_name = struct_type->name;
            } else if (field_expr->object->type->kind == TypeKind::REFERENCE) {
                // 处理引用类型：需要解引用
                auto ref_type = std::dynamic_pointer_cast<ReferenceType>(field_expr->object->type);
                if (ref_type && ref_type->referenced_type) {
                    if (auto struct_type =
                            std::dynamic_pointer_cast<StructType>(ref_type->referenced_type)) {
                        type_name = struct_type->name;
                    }
                }
            } else if (field_expr->object->type->kind == TypeKind::RAW_POINTER) {
                // 处理指针类型：需要解引用
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

        // 应用name mangling
        func_name = type_name + "_" + method_name;

        // self参数：获取对象的类型
        std::string obj_type_str;
        if (field_expr->object->type->kind == TypeKind::REFERENCE) {
            // REFERENCE类型在IR中已经是指针，不需要再加*
            auto ref_type = std::dynamic_pointer_cast<ReferenceType>(field_expr->object->type);
            if (ref_type && ref_type->referenced_type) {
                obj_type_str = type_mapper_.map(ref_type->referenced_type.get()) + "*";
            }
        } else {
            // 其他类型需要加*变成指针
            obj_type_str = type_mapper_.map(field_expr->object->type.get()) + "*";
        }
        self_args.push_back({obj_type_str, obj_ptr});
    } else {
        // TODO: 处理其他复杂的 callee
        store_expr_result(node, "");
        return;
    }

    // 2.5 检查是否为内置函数
    if (handle_builtin_function(node, func_name, args)) {
        return; // 已处理
    }

    // 3. 获取返回类型
    std::string ret_type_str = "void";
    bool ret_is_aggregate = false;
    bool use_sret = false;

    if (node->type) {
        ret_type_str = type_mapper_.map(node->type.get());
        ret_is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
        
        // 检查是否使用 sret 优化
        if (should_use_sret_optimization(func_name, node->type.get())) {
            use_sret = true;
        }
    }

    // 合并self参数和普通参数
    // self参数（如果有）应该在最前面
    std::vector<std::pair<std::string, std::string>> all_args;
    
    // 如果使用 sret，先 alloca 并添加为第一个参数
    std::string sret_alloca;
    if (use_sret) {
        sret_alloca = emitter_.emit_alloca(ret_type_str);
        all_args.push_back({ret_type_str + "*", sret_alloca});
    }
    
    all_args.insert(all_args.end(), self_args.begin(), self_args.end());
    all_args.insert(all_args.end(), args.begin(), args.end());

    // 4. 生成 call 指令
    if (use_sret) {
        // sret 模式：调用返回 void，结果已在 sret_alloca 中
        emitter_.emit_call_void(func_name, all_args);
        store_expr_result(node, sret_alloca);
    } else if (ret_type_str == "void") {
        emitter_.emit_call_void(func_name, all_args);
        store_expr_result(node, ""); // void 函数没有返回值
    } else {
        std::string result = emitter_.emit_call(ret_type_str, func_name, all_args);

        // 如果返回聚合类型，需要分配空间并store
        if (ret_is_aggregate) {
            // 为返回值分配空间
            std::string alloca_ptr = emitter_.emit_alloca(ret_type_str);
            // 存储返回的聚合值
            emitter_.emit_store(ret_type_str, result, alloca_ptr);
            // 返回指针（保持与ArrayLiteral/StructInitializer一致）
            store_expr_result(node, alloca_ptr);
        } else {
            // 基础类型：直接使用返回值
            store_expr_result(node, result);
        }
    }
}

void IRGenerator::visit(AssignmentExpr *node) {
    // x = value;

    // 1. 计算右值
    node->value->accept(this);
    std::string value_var = get_expr_result(node->value.get());

    if (value_var.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 2. 获取左值地址
    if (auto var_expr = dynamic_cast<VariableExpr *>(node->target.get())) {
        // 简单变量赋值
        std::string var_name = var_expr->name.lexeme;
        VariableInfo *var_info = value_manager_.lookup_variable(var_name);

        if (!var_info) {
            store_expr_result(node, "");
            return;
        }

        // 检查可变性
        if (!var_info->is_mutable) {
            // 错误：尝试修改不可变变量（应该在语义分析中捕获）
            store_expr_result(node, "");
            return;
        }

        // 存储值
        if (node->value->type && var_expr->type) {
            std::string value_type_str = type_mapper_.map(node->value->type.get());
            std::string target_type_str = type_mapper_.map(var_expr->type.get());

            // 如果值是聚合类型且结果是指针，需要先load
            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                // 聚合类型：总是load值（因为value_var是指针）
                value_to_store = emitter_.emit_load(value_type_str, value_var);
            }

            // 类型转换：如果赋值bool到整数类型，需要zext
            if (node->value->type->kind == TypeKind::BOOL &&
                (var_expr->type->kind == TypeKind::I32 ||
                 var_expr->type->kind == TypeKind::USIZE)) {
                value_to_store = emitter_.emit_zext("i1", value_to_store, target_type_str);
                value_type_str = target_type_str; // 更新值的类型为目标类型
            }

            emitter_.emit_store(target_type_str, value_to_store, var_info->alloca_name);
        }

        // 赋值表达式在 Rust 中返回 ()
        store_expr_result(node, "");
    } else if (auto index_expr = dynamic_cast<IndexExpr *>(node->target.get())) {
        // 数组元素赋值: arr[i] = value
        // 设置左值生成标志
        bool old_flag = generating_lvalue_;
        generating_lvalue_ = true;
        index_expr->accept(this);
        generating_lvalue_ = old_flag;

        std::string elem_ptr = get_expr_result(index_expr);

        if (!elem_ptr.empty() && node->value->type) {
            std::string type_str = type_mapper_.map(node->value->type.get());

            // 如果值是聚合类型，需要先load
            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                // 聚合类型：总是load值（因为value_var是指针）
                value_to_store = emitter_.emit_load(type_str, value_var);
            }

            emitter_.emit_store(type_str, value_to_store, elem_ptr);
        }

        store_expr_result(node, "");
    } else if (auto field_expr = dynamic_cast<FieldAccessExpr *>(node->target.get())) {
        // 结构体字段赋值: obj.field = value
        // 设置左值生成标志
        bool old_flag = generating_lvalue_;
        generating_lvalue_ = true;
        field_expr->accept(this);
        generating_lvalue_ = old_flag;

        std::string field_ptr = get_expr_result(field_expr);

        if (!field_ptr.empty() && node->value->type) {
            std::string type_str = type_mapper_.map(node->value->type.get());

            // 如果值是聚合类型，需要先load
            bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                       node->value->type->kind == TypeKind::STRUCT);
            std::string value_to_store = value_var;

            if (value_is_aggregate) {
                // 聚合类型：总是load值（因为value_var是指针）
                value_to_store = emitter_.emit_load(type_str, value_var);
            }

            emitter_.emit_store(type_str, value_to_store, field_ptr);
        }

        store_expr_result(node, "");
    } else if (auto unary_expr = dynamic_cast<UnaryExpr *>(node->target.get())) {
        // 解引用赋值: *ptr = value
        if (unary_expr->op.type == TokenType::STAR) {
            // 计算指针值
            unary_expr->right->accept(this);
            std::string ptr_value = get_expr_result(unary_expr->right.get());

            if (!ptr_value.empty() && node->value->type) {
                std::string value_type_str = type_mapper_.map(node->value->type.get());
                std::string value_to_store = value_var;

                // 如果值是聚合类型，需要先load
                bool value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                                           node->value->type->kind == TypeKind::STRUCT);

                if (value_is_aggregate) {
                    value_to_store = emitter_.emit_load(value_type_str, value_var);
                }

                // 直接store到指针指向的位置
                emitter_.emit_store(value_type_str, value_to_store, ptr_value);
            }
        }

        store_expr_result(node, "");
    } else {
        // 其他情况（TODO: 元组元素赋值等）
        store_expr_result(node, "");
    }
}

void IRGenerator::visit(GroupingExpr *node) {
    // (expr) -> 直接处理内部表达式
    if (node->expression) {
        node->expression->accept(this);

        // 继承内部表达式的结果
        std::string result = get_expr_result(node->expression.get());
        store_expr_result(node, result);
    }
}

void IRGenerator::visit(BlockExpr *node) {
    // 块表达式 { ... }
    if (node->block_stmt) {
        node->block_stmt->accept(this);

        // 如果块有最终表达式，获取其结果
        if (node->block_stmt->final_expr.has_value()) {
            auto final_expr = node->block_stmt->final_expr.value();
            if (final_expr) {
                std::string result = get_expr_result(final_expr.get());
                store_expr_result(node, result);

                // 如果final_expr的结果已经是值（对于聚合类型），也标记此BlockExpr
                if (loaded_aggregate_results_.find(final_expr.get()) !=
                    loaded_aggregate_results_.end()) {
                    loaded_aggregate_results_.insert(node);
                }
                return;
            }
        }
    }

    // 否则块表达式返回 unit
    store_expr_result(node, "");
}

void IRGenerator::visit(AsExpr *node) {
    // 类型转换: expr as Type
    // 支持整数类型之间的转换: i32, u32, bool, usize等

    // 1. 计算源表达式
    node->expression->accept(this);
    std::string source_value = get_expr_result(node->expression.get());

    if (source_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 2. 获取源类型和目标类型
    auto source_type = node->expression->type;
    auto target_type = node->type;

    if (!source_type || !target_type) {
        store_expr_result(node, "");
        return;
    }

    // 3. 映射到LLVM类型
    std::string source_llvm_type = type_mapper_.map(source_type.get());
    std::string target_llvm_type = type_mapper_.map(target_type.get());

    // 4. 如果LLVM类型相同，不需要转换（例如i32和u32都映射为i32）
    if (source_llvm_type == target_llvm_type) {
        store_expr_result(node, source_value);
        return;
    }

    // 5. 获取位宽信息
    int source_bits = get_integer_bits(source_type->kind);
    int target_bits = get_integer_bits(target_type->kind);

    // 6. 执行转换
    std::string result;

    if (source_bits == target_bits) {
        // 位宽相同，直接使用（LLVM中有符号和无符号都是相同的整数类型）
        result = source_value;
    } else if (source_bits < target_bits) {
        // 位宽扩展
        bool is_signed =
            (source_type->kind == TypeKind::I32 || source_type->kind == TypeKind::ISIZE);

        if (is_signed) {
            // 有符号扩展: sext
            result = emitter_.emit_sext(source_llvm_type, source_value, target_llvm_type);
        } else {
            // 无符号扩展: zext (包括bool)
            result = emitter_.emit_zext(source_llvm_type, source_value, target_llvm_type);
        }
    } else {
        // 位宽缩减: trunc
        result = emitter_.emit_trunc(source_llvm_type, source_value, target_llvm_type);
    }

    store_expr_result(node, result);
}

void IRGenerator::visit(ReferenceExpr *node) {
    // & expression -> 返回表达式的地址
    // 对于聚合类型，expression已经是指针，直接返回
    // 对于基础类型，需要特殊处理

    // 注意：取地址操作需要获取表达式的地址（左值），不应load值
    // 因此，我们临时设置generating_lvalue_=true来获取表达式的地址
    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true; // 取地址必须返回指针（地址）
    node->expression->accept(this);
    generating_lvalue_ = was_generating_lvalue; // 恢复原来的标志

    std::string value = get_expr_result(node->expression.get());

    if (value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 如果ReferenceExpr没有类型信息，尝试从expression推导
    if (!node->type && node->expression->type) {
        // 创建一个ReferenceType (参数顺序: referenced_type, is_mutable)
        node->type = std::make_shared<ReferenceType>(node->expression->type, false);
    }

    // 对于聚合类型（数组、结构体），表达式的结果已经是指针
    if (node->expression->type) {
        bool is_aggregate = (node->expression->type->kind == TypeKind::ARRAY ||
                             node->expression->type->kind == TypeKind::STRUCT);

        if (is_aggregate) {
            // 聚合类型已经是指针，直接传递
            store_expr_result(node, value);
        } else {
            // 基础类型需要取地址（但一般都已经有alloca）
            // 目前简化处理：如果value已经是指针形式，直接用
            store_expr_result(node, value);
        }
    } else {
        store_expr_result(node, value);
    }
}

// ========== 未实现的 Expression Visitors ==========

void IRGenerator::visit(CompoundAssignmentExpr *node) {
    // 复合赋值：x += value  等价于  x = x + value
    // 支持的运算符：+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=

    // 1. 获取左值地址
    std::string target_ptr;
    std::string target_type_str;

    if (auto var_expr = dynamic_cast<VariableExpr *>(node->target.get())) {
        // 简单变量
        std::string var_name = var_expr->name.lexeme;
        VariableInfo *var_info = value_manager_.lookup_variable(var_name);

        if (!var_info) {
            store_expr_result(node, "");
            return;
        }

        target_ptr = var_info->alloca_name;
        // 去掉指针类型的*后缀，得到基础类型
        target_type_str = var_info->type_str;
        if (!target_type_str.empty() && target_type_str.back() == '*') {
            target_type_str.pop_back();
        }
    } else if (auto index_expr = dynamic_cast<IndexExpr *>(node->target.get())) {
        // 数组元素：计算元素地址（需要左值）
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
        // 结构体字段：计算字段地址（需要左值）
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
        // 解引用：*ptr += value
        if (unary_expr->op.type == TokenType::STAR) {
            // 计算指针值（指针本身，不解引用）
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

    // 2. 加载当前值
    std::string current_value = emitter_.emit_load(target_type_str, target_ptr);

    // 3. 计算右值
    node->value->accept(this);
    std::string rhs_value = get_expr_result(node->value.get());

    if (rhs_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 4. 根据运算符执行操作
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

    // 5. 存储结果
    emitter_.emit_store(target_type_str, result, target_ptr);

    // 复合赋值表达式没有返回值（或返回unit）
    store_expr_result(node, "");
}

void IRGenerator::visit(UnderscoreExpr *node) {}
void IRGenerator::visit(UnitExpr *node) {}
void IRGenerator::visit(TupleExpr *node) {}
void IRGenerator::visit(MatchExpr *node) {}
void IRGenerator::visit(PathExpr *node) {}
