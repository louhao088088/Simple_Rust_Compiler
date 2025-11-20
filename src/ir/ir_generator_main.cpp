/**
 * @file ir_generator.cpp (主文件 - 重构后)
 * @brief IR生成器 - 主入口和Item处理
 *
 * 本文件包含IR生成器的主入口和顶层Item处理：
 * - generate(): 主入口函数
 * - visit_item(): Item分发
 * - visit_function_decl(): 函数声明处理
 * - visit_struct_decl(): 结构体声明处理
 * - visit_const_decl(): 常量声明处理
 * - visit_impl_block(): impl块处理
 *
 * 其他模块：
 * - ir_generator_builtins.cpp: 内置函数支持
 * - ir_generator_helpers.cpp: 辅助方法
 * - ir_generator_statements.cpp: Statement处理
 * - ir_generator_expressions.cpp: 基础Expression处理
 * - ir_generator_control_flow.cpp: 控制流Expression处理
 * - ir_generator_complex_exprs.cpp: 复杂Expression处理（数组/结构体）
 */

#include "ir_generator.h"

#include <cassert>

IRGenerator::IRGenerator(BuiltinTypes &builtin_types)
    : emitter_("main_module"), type_mapper_(builtin_types) {}

std::string IRGenerator::generate(Program *program) {
    if (!program) {
        return "";
    }

    // 第一遍：收集所有struct定义（包括local struct）
    collect_all_structs(program);

    // 生成所有收集到的struct类型定义
    for (StructDecl *struct_decl : local_structs_set_) {
        visit_struct_decl(struct_decl);
    }

    // 声明内置函数需要的C库函数
    emit_builtin_declarations();

    // 第二遍：遍历所有顶层 items
    for (const auto &item : program->items) {
        visit_item(item.get());
    }

    // 返回生成的完整 IR
    return emitter_.get_ir_string();
}

// ========== Item 处理 ==========

void IRGenerator::visit_item(Item *item) {
    if (auto fn_decl = dynamic_cast<FnDecl *>(item)) {
        visit_function_decl(fn_decl);
    } else if (auto struct_decl = dynamic_cast<StructDecl *>(item)) {
        // Struct已经在collect_all_structs阶段处理并生成
        // 这里不需要重复生成
        // visit_struct_decl(struct_decl);
    } else if (auto const_decl = dynamic_cast<ConstDecl *>(item)) {
        visit_const_decl(const_decl);
    } else if (auto impl_block = dynamic_cast<ImplBlock *>(item)) {
        visit_impl_block(impl_block);
    }
}

void IRGenerator::visit_function_decl(FnDecl *node) {
    // 保存当前的嵌套函数列表（用于处理多层嵌套）
    std::vector<FnDecl *> outer_nested_functions = std::move(nested_functions_);
    nested_functions_.clear();

    // 标记进入函数体
    bool was_inside = inside_function_body_;
    inside_function_body_ = true;

    // 1. 获取返回类型
    std::string ret_type_str = "void";
    Type *return_type_ptr = nullptr;
    bool use_sret = false;

    if (node->return_type.has_value()) {
        auto ret_type_node = node->return_type.value();
        if (ret_type_node && ret_type_node->resolved_type) {
            return_type_ptr = ret_type_node->resolved_type.get();
            ret_type_str = type_mapper_.map(return_type_ptr);
        }
    }

    // 2. 构建参数列表 (type, name)
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::string> param_names;
    std::vector<bool> param_is_aggregate; // 标记参数是否为聚合类型

    // 检查是否使用 sret 优化
    std::string func_name = node->name.lexeme;
    if (return_type_ptr && should_use_sret_optimization(func_name, return_type_ptr)) {
        use_sret = true;
        // 添加 sret 参数作为第一个参数
        // 使用特殊名称避免与用户参数冲突
        params.push_back({ret_type_str + "*", "sret_ptr"});
        param_names.push_back("sret_ptr");
        // param_is_aggregate 仅用于跟踪用户参数，不需要为 sret 添加条目
    }

    for (const auto &param : node->params) {
        if (param->type && param->type->resolved_type) {
            auto resolved_type = param->type->resolved_type.get();
            std::string param_type_str = type_mapper_.map(resolved_type);

            // 检查是否为聚合类型（数组或结构体）
            bool is_aggregate =
                (resolved_type->kind == TypeKind::ARRAY || resolved_type->kind == TypeKind::STRUCT);

            // 获取参数名
            if (auto id_pattern = dynamic_cast<IdentifierPattern *>(param->pattern.get())) {
                std::string param_name = id_pattern->name.lexeme;

                // 检查是否为可变引用类型
                bool is_mut_ref = false;
                if (resolved_type->kind == TypeKind::REFERENCE) {
                    if (auto ref_type = dynamic_cast<ReferenceType *>(resolved_type)) {
                        if (ref_type->is_mutable) {
                            is_mut_ref = true;
                        }
                    }
                }

                // 聚合类型参数使用指针传递
                if (is_aggregate) {
                    params.push_back({param_type_str + "*", param_name});
                } else {
                    std::string type_with_attr = param_type_str;
                    // 优化：可变引用参数添加 noalias 属性，帮助 LLVM 优化
                    if (is_mut_ref) {
                        type_with_attr += " noalias";
                    }
                    params.push_back({type_with_attr, param_name});
                }

                param_names.push_back(param_name);
                param_is_aggregate.push_back(is_aggregate);
            }
        }
    }

    // 3. 开始函数定义
    // func_name 已在上面定义
    // 如果使用 sret，返回类型改为 void
    std::string actual_ret_type = use_sret ? "void" : ret_type_str;

    // 设置当前函数的 sret 状态
    current_function_uses_sret_ = use_sret;

    emitter_.begin_function(actual_ret_type, func_name, params);
    begin_block("bb.entry"); // 使用bb.entry避免与参数名entry冲突

    // 4. 重置临时变量计数器
    emitter_.reset_temp_counter();

    // 5. 进入函数作用域
    value_manager_.enter_scope();

    // 如果使用 sret，将 self 注册为特殊变量
    size_t param_start_index = 0;
    if (use_sret) {
        value_manager_.define_variable("__sret_self", "%sret_ptr", ret_type_str + "*", false);
        param_start_index = 1; // 真实参数从索引 1 开始
    }

    // 6. 处理参数: alloca + store + 注册到 ValueManager
    for (size_t i = 0; i < node->params.size(); ++i) {
        const auto &param = node->params[i];

        if (auto id_pattern = dynamic_cast<IdentifierPattern *>(param->pattern.get())) {
            std::string param_name = id_pattern->name.lexeme;
            std::string param_ir_name = "%" + param_name;

            if (param->type && param->type->resolved_type) {
                std::string param_type_str = type_mapper_.map(param->type->resolved_type.get());
                bool is_mutable = id_pattern->is_mutable;

                // 检查是否为引用类型
                // 引用类型(&T)已经被映射为指针(T*)，不需要再创建alloca
                bool is_reference = (param->type->resolved_type->kind == TypeKind::REFERENCE);
                bool is_aggregate = (i < param_is_aggregate.size() && param_is_aggregate[i]);

                if (is_reference) {
                    // 引用参数: 已经是指针，直接注册
                    value_manager_.define_variable(param_name, param_ir_name, param_type_str,
                                                   is_mutable);
                } else if (is_aggregate) {
                    // 数组/结构体按值传递: 参数是指针，但需要拷贝到本地
                    // 创建本地alloca
                    std::string local_alloca = emitter_.emit_alloca(param_type_str);

                    // 使用memcpy拷贝数组/结构体内容
                    auto resolved = param->type->resolved_type.get();
                    size_t size_bytes = get_type_size(resolved);

                    // 目标是指针类型 (e.g. %Point*)
                    std::string ptr_type = param_type_str + "*";
                    emitter_.emit_memcpy(local_alloca, param_ir_name, size_bytes, ptr_type);

                    // 注册本地变量
                    value_manager_.define_variable(param_name, local_alloca, param_type_str + "*",
                                                   is_mutable);
                } else {
                    // 基础类型参数：需要 alloca + store
                    // 为参数创建 alloca
                    std::string alloca_name = emitter_.emit_alloca(param_type_str);

                    // 将参数值存入 alloca
                    emitter_.emit_store(param_type_str, param_ir_name, alloca_name);

                    // 注册到 ValueManager (存储指针类型)
                    std::string ptr_type_str = param_type_str + "*";
                    value_manager_.define_variable(param_name, alloca_name, ptr_type_str,
                                                   is_mutable);
                }
            }
        }
    }

    // 7. 生成函数体
    if (node->body.has_value()) {
        auto body = node->body.value();
        if (body) {
            body->accept(this);

            // 获取body的最终表达式结果（如果有）
            std::string body_result;
            if (body->final_expr.has_value()) {
                auto final_expr = body->final_expr.value();
                if (final_expr) {
                    body_result = get_expr_result(final_expr.get());
                }
            }

            // 如果body有返回值且当前块未终止，生成ret指令
            if (!current_block_terminated_) {
                if (use_sret) {
                    // sret 模式：需要将函数体结果复制到 sret_ptr
                    if (!body_result.empty() && return_type_ptr) {
                        // body_result 是指针，指向要返回的结构体
                        // 将其内容复制到 sret_ptr
                        size_t size_bytes = get_type_size(return_type_ptr);
                        std::string ptr_type = ret_type_str + "*";
                        emitter_.emit_memcpy("%sret_ptr", body_result, size_bytes, ptr_type);
                    }
                    emitter_.emit_ret_void();
                } else if (ret_type_str == "void") {
                    emitter_.emit_ret_void();
                } else if (!body_result.empty()) {
                    // 检查返回类型是否为聚合类型
                    bool ret_is_aggregate = false;
                    if (node->return_type.has_value()) {
                        auto ret_type_node = node->return_type.value();
                        if (ret_type_node && ret_type_node->resolved_type) {
                            auto resolved = ret_type_node->resolved_type.get();
                            ret_is_aggregate = (resolved->kind == TypeKind::ARRAY ||
                                                resolved->kind == TypeKind::STRUCT);
                        }
                    }

                    // 聚合类型返回值：需要load整个值
                    if (ret_is_aggregate) {
                        // body_result是指针，需要load整个聚合值
                        std::string loaded_value = emitter_.emit_load(ret_type_str, body_result);
                        emitter_.emit_ret(ret_type_str, loaded_value);
                    } else {
                        // 基础类型：直接返回
                        emitter_.emit_ret(ret_type_str, body_result);
                    }
                } else {
                    // 无返回值，返回未定义（实际应该报错，但为了兼容性返回0）
                    emitter_.emit_ret(ret_type_str, "0");
                }
            }
        }
    } else {
        // 没有函数体，添加默认返回
        if (ret_type_str == "void") {
            emitter_.emit_ret_void();
        }
    }

    // 8. 重置终止标志和 sret 状态
    current_block_terminated_ = false;
    current_function_uses_sret_ = false;

    // 9. 退出函数作用域
    value_manager_.exit_scope();

    // 10. 结束函数定义
    emitter_.end_function();
    emitter_.emit_blank_line();

    // 11. 恢复嵌套函数状态
    inside_function_body_ = was_inside;

    // 12. 处理在此函数体内定义的嵌套函数（提升为顶层函数）
    for (FnDecl *nested_fn : nested_functions_) {
        visit_function_decl(nested_fn);
    }

    // 13. 恢复外层的嵌套函数列表
    nested_functions_ = std::move(outer_nested_functions);
}

void IRGenerator::visit_struct_decl(StructDecl *node) {
    // TODO: 实现结构体生成
    if (!node->resolved_symbol || !node->resolved_symbol->type) {
        return;
    }

    auto struct_type = std::dynamic_pointer_cast<StructType>(node->resolved_symbol->type);
    if (!struct_type) {
        return;
    }

    // 生成字段类型列表（使用 StructType 的 field_order 保证顺序正确）
    std::vector<std::string> field_types;
    for (const auto &field_name : struct_type->field_order) {
        auto it = struct_type->fields.find(field_name);
        if (it != struct_type->fields.end()) {
            std::string field_type_str = type_mapper_.map(it->second.get());
            field_types.push_back(field_type_str);
        }
    }

    // 生成结构体定义
    emitter_.emit_struct_type(struct_type->name, field_types);
}

void IRGenerator::visit_const_decl(ConstDecl *node) {
    // const常量生成为全局常量
    // 例如: const MAX: i32 = 100;
    // 生成: @MAX = constant i32 100

    if (!node->type || !node->type->resolved_type) {
        return;
    }

    // 获取类型
    std::string llvm_type = type_mapper_.map(node->type->resolved_type.get());
    std::string const_name = node->name.lexeme;

    // 尝试编译时求值常量表达式
    std::string value_str;
    bool has_value = evaluate_const_expr(node->value.get(), value_str);

    // 如果求值成功，生成全局常量并记录到常量表
    if (has_value) {
        // 生成全局常量: @MAX = constant i32 100
        emitter_.emit_global_variable(const_name, llvm_type, value_str, true);

        // 记录到常量表，供后续常量引用
        const_values_[const_name] = value_str;
    } else {
        // 调试：求值失败时输出警告
        std::cerr << "Warning: Failed to evaluate constant expression for: " << const_name
                  << std::endl;
    }
}
void IRGenerator::visit_impl_block(ImplBlock *node) {
    // impl块处理：生成impl块中的所有方法
    // 方法被当作普通函数生成，但函数名需要加上类型前缀（name mangling）

    if (!node->target_type || !node->target_type->resolved_type) {
        return;
    }

    // 获取目标类型名称（用于name mangling）
    std::string type_name;
    if (auto struct_type =
            std::dynamic_pointer_cast<StructType>(node->target_type->resolved_type)) {
        type_name = struct_type->name;
    } else {
        return; // 只支持struct的impl块
    }

    // 遍历impl块中的所有项（通常是方法）
    for (const auto &item : node->implemented_items) {
        if (auto fn_decl = dynamic_cast<FnDecl *>(item.get())) {
            // 保存原函数名
            std::string original_name = fn_decl->name.lexeme;

            // 应用name mangling：TypeName_MethodName
            std::string mangled_name = type_name + "_" + original_name;

            // 临时修改函数名以生成带前缀的IR
            Token original_token = fn_decl->name;
            fn_decl->name.lexeme = mangled_name;

            // 生成函数
            visit_function_decl(fn_decl);

            // 恢复原函数名
            fn_decl->name = original_token;
        }
    }
}

// ========== Struct收集辅助方法 ==========

void IRGenerator::collect_all_structs(Program *program) {
    // 收集顶层struct
    for (const auto &item : program->items) {
        if (auto struct_decl = dynamic_cast<StructDecl *>(item.get())) {
            local_structs_set_.insert(struct_decl);
        } else if (auto fn_decl = dynamic_cast<FnDecl *>(item.get())) {
            // 收集函数体内的local struct
            if (fn_decl->body.has_value() && fn_decl->body.value()) {
                collect_structs_from_stmt(fn_decl->body.value().get());
            }
        }
    }
}

void IRGenerator::collect_structs_from_stmt(Stmt *stmt) {
    if (!stmt)
        return;

    if (auto block_stmt = dynamic_cast<BlockStmt *>(stmt)) {
        // 遍历块中的所有语句
        for (const auto &s : block_stmt->statements) {
            collect_structs_from_stmt(s.get());
        }
    } else if (auto item_stmt = dynamic_cast<ItemStmt *>(stmt)) {
        // 检查是否是struct定义
        if (item_stmt->item) {
            if (auto struct_decl = dynamic_cast<StructDecl *>(item_stmt->item.get())) {
                local_structs_set_.insert(struct_decl);
            } else if (auto fn_decl = dynamic_cast<FnDecl *>(item_stmt->item.get())) {
                // 嵌套函数的函数体也可能包含struct
                if (fn_decl->body.has_value() && fn_decl->body.value()) {
                    collect_structs_from_stmt(fn_decl->body.value().get());
                }
            }
        }
    }
    // 其他类型的语句不包含struct定义
}
