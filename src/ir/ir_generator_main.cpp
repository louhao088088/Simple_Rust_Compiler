#include "ir_generator.h"

#include <cassert>

IRGenerator::IRGenerator(BuiltinTypes &builtin_types)
    : emitter_("main_module"), type_mapper_(builtin_types) {}

/**
 * Generate complete LLVM IR for the entire program.
 *
 * Process:
 * 1. Collect and emit all struct type definitions (including nested structs)
 * 2. Emit built-in function declarations (printf, scanf, exit)
 * 3. Process all top-level items (functions, consts, impl blocks)
 * 4. Return complete IR module as text
 *
 * @param program The program AST root node
 * @return Complete LLVM IR module as string
 */
std::string IRGenerator::generate(Program *program) {
    if (!program) {
        return "";
    }

    collect_all_structs(program);

    for (StructDecl *struct_decl : local_structs_set_) {
        visit_struct_decl(struct_decl);
    }

    emit_builtin_declarations();

    for (const auto &item : program->items) {
        visit_item(item.get());
    }

    return emitter_.get_ir_string();
}

// Dispatch to appropriate visitor based on item type.
void IRGenerator::visit_item(Item *item) {
    if (auto fn_decl = dynamic_cast<FnDecl *>(item)) {
        visit_function_decl(fn_decl);
    } else if (auto struct_decl = dynamic_cast<StructDecl *>(item)) {
    } else if (auto const_decl = dynamic_cast<ConstDecl *>(item)) {
        visit_const_decl(const_decl);
    } else if (auto impl_block = dynamic_cast<ImplBlock *>(item)) {
        visit_impl_block(impl_block);
    }
}

/**
 * Generate IR for a function declaration.
 *
 * Handles:
 * - Function signature generation with proper parameter types
 * - SRET optimization for large struct returns
 * - Parameter passing strategies:
 *   * Scalar types: pass by value, alloca + store in function
 *   * Aggregate types (arrays/structs): pass by pointer, memcpy to local
 *   * References: pass pointer directly, no alloca needed
 * - Function body generation with proper scoping
 * - Return value handling (direct return or SRET copy)
 *
 * Optimizations:
 * - All allocas hoisted to entry block by IREmitter
 * - Mutable reference parameters marked with noalias attribute
 * - Large struct returns use SRET (return via pointer parameter)
 *
 * @param node The function declaration AST node
 */
void IRGenerator::visit_function_decl(FnDecl *node) {
    std::vector<FnDecl *> outer_nested_functions = std::move(nested_functions_);
    nested_functions_.clear();

    bool was_inside = inside_function_body_;
    inside_function_body_ = true;

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

    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::string> param_names;
    std::vector<bool> param_is_aggregate;

    std::string func_name = node->name.lexeme;
    if (return_type_ptr && should_use_sret_optimization(func_name, return_type_ptr)) {
        use_sret = true;
        params.push_back({ret_type_str + "*", "sret_ptr"});
        param_names.push_back("sret_ptr");
    }

    for (const auto &param : node->params) {
        if (param->type && param->type->resolved_type) {
            auto resolved_type = param->type->resolved_type.get();
            std::string param_type_str = type_mapper_.map(resolved_type);

            bool is_aggregate =
                (resolved_type->kind == TypeKind::ARRAY || resolved_type->kind == TypeKind::STRUCT);

            if (auto id_pattern = dynamic_cast<IdentifierPattern *>(param->pattern.get())) {
                std::string param_name = id_pattern->name.lexeme;

                bool is_mut_ref = false;
                if (resolved_type->kind == TypeKind::REFERENCE) {
                    if (auto ref_type = dynamic_cast<ReferenceType *>(resolved_type)) {
                        if (ref_type->is_mutable) {
                            is_mut_ref = true;
                        }
                    }
                }

                if (is_aggregate) {
                    params.push_back({param_type_str + "*", param_name});
                } else {
                    std::string type_with_attr = param_type_str;
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

    std::string actual_ret_type = use_sret ? "void" : ret_type_str;

    current_function_uses_sret_ = use_sret;
    current_function_return_type_str_ = use_sret ? "void" : ret_type_str;

    emitter_.begin_function(actual_ret_type, func_name, params);
    begin_block("bb.entry");

    emitter_.reset_temp_counter();

    value_manager_.enter_scope();

    size_t param_start_index = 0;
    if (use_sret) {
        value_manager_.define_variable("__sret_self", "%sret_ptr", ret_type_str + "*", false);
        param_start_index = 1;
    }

    for (size_t i = 0; i < node->params.size(); ++i) {
        const auto &param = node->params[i];

        if (auto id_pattern = dynamic_cast<IdentifierPattern *>(param->pattern.get())) {
            std::string param_name = id_pattern->name.lexeme;
            std::string param_ir_name = "%" + param_name;

            if (param->type && param->type->resolved_type) {
                std::string param_type_str = type_mapper_.map(param->type->resolved_type.get());
                bool is_mutable = id_pattern->is_mutable;

                bool is_reference = (param->type->resolved_type->kind == TypeKind::REFERENCE);
                bool is_aggregate = (i < param_is_aggregate.size() && param_is_aggregate[i]);

                if (is_reference) {
                    value_manager_.define_variable(param_name, param_ir_name, param_type_str,
                                                   is_mutable);
                } else if (is_aggregate) {
                    std::string local_alloca = emitter_.emit_alloca(param_type_str);

                    auto resolved = param->type->resolved_type.get();
                    size_t size_bytes = get_type_size(resolved);

                    std::string ptr_type = param_type_str + "*";
                    emitter_.emit_memcpy(local_alloca, param_ir_name, size_bytes, ptr_type);

                    value_manager_.define_variable(param_name, local_alloca, param_type_str + "*",
                                                   is_mutable);
                } else {
                    std::string alloca_name = emitter_.emit_alloca(param_type_str);

                    emitter_.emit_store(param_type_str, param_ir_name, alloca_name);

                    std::string ptr_type_str = param_type_str + "*";
                    value_manager_.define_variable(param_name, alloca_name, ptr_type_str,
                                                   is_mutable);
                }
            }
        }
    }

    if (node->body.has_value()) {
        auto body = node->body.value();
        if (body) {
            body->accept(this);

            std::string body_result;
            if (body->final_expr.has_value()) {
                auto final_expr = body->final_expr.value();
                if (final_expr) {
                    body_result = get_expr_result(final_expr.get());
                }
            }

            if (!current_block_terminated_) {
                if (use_sret) {
                    if (!body_result.empty() && return_type_ptr) {
                        size_t size_bytes = get_type_size(return_type_ptr);
                        std::string ptr_type = ret_type_str + "*";
                        emitter_.emit_memcpy("%sret_ptr", body_result, size_bytes, ptr_type);
                    }
                    emitter_.emit_ret_void();
                } else if (ret_type_str == "void") {
                    emitter_.emit_ret_void();
                } else if (!body_result.empty()) {
                    bool ret_is_aggregate = false;
                    if (node->return_type.has_value()) {
                        auto ret_type_node = node->return_type.value();
                        if (ret_type_node && ret_type_node->resolved_type) {
                            auto resolved = ret_type_node->resolved_type.get();
                            ret_is_aggregate = (resolved->kind == TypeKind::ARRAY ||
                                                resolved->kind == TypeKind::STRUCT);
                        }
                    }

                    if (ret_is_aggregate) {
                        std::string loaded_value = emitter_.emit_load(ret_type_str, body_result);
                        emitter_.emit_ret(ret_type_str, loaded_value);
                    } else {
                        emitter_.emit_ret(ret_type_str, body_result);
                    }
                } else {
                    emitter_.emit_ret(ret_type_str, "0");
                }
            }
        }
    } else {
        if (ret_type_str == "void") {
            emitter_.emit_ret_void();
        }
    }

    current_block_terminated_ = false;
    current_function_uses_sret_ = false;
    current_function_return_type_str_ = "";

    value_manager_.exit_scope();

    emitter_.end_function();
    emitter_.emit_blank_line();

    inside_function_body_ = was_inside;

    for (FnDecl *nested_fn : nested_functions_) {
        visit_function_decl(nested_fn);
    }

    nested_functions_ = std::move(outer_nested_functions);
}

// Generate IR for struct type declarations.
void IRGenerator::visit_struct_decl(StructDecl *node) {
    if (!node->resolved_symbol || !node->resolved_symbol->type) {
        return;
    }

    auto struct_type = std::dynamic_pointer_cast<StructType>(node->resolved_symbol->type);
    if (!struct_type) {
        return;
    }

    std::vector<std::string> field_types;
    for (const auto &field_name : struct_type->field_order) {
        auto it = struct_type->fields.find(field_name);
        if (it != struct_type->fields.end()) {
            std::string field_type_str = type_mapper_.map(it->second.get());
            field_types.push_back(field_type_str);
        }
    }

    emitter_.emit_struct_type(struct_type->name, field_types);
}

// Generate IR for const declarations.
void IRGenerator::visit_const_decl(ConstDecl *node) {

    if (!node->type || !node->type->resolved_type) {
        return;
    }

    std::string llvm_type = type_mapper_.map(node->type->resolved_type.get());
    std::string const_name = node->name.lexeme;

    std::string value_str;
    bool has_value = evaluate_const_expr(node->value.get(), value_str);

    if (has_value) {
        emitter_.emit_global_variable(const_name, llvm_type, value_str, true);

        const_values_[const_name] = value_str;
    } else {
        std::cerr << "Warning: Failed to evaluate constant expression for: " << const_name
                  << std::endl;
    }
}
// Generate IR for impl blocks.
void IRGenerator::visit_impl_block(ImplBlock *node) {

    if (!node->target_type || !node->target_type->resolved_type) {
        return;
    }

    std::string type_name;
    if (auto struct_type =
            std::dynamic_pointer_cast<StructType>(node->target_type->resolved_type)) {
        type_name = struct_type->name;
    } else {
        return;
    }

    for (const auto &item : node->implemented_items) {
        if (auto fn_decl = dynamic_cast<FnDecl *>(item.get())) {
            std::string original_name = fn_decl->name.lexeme;

            std::string mangled_name = type_name + "_" + original_name;

            Token original_token = fn_decl->name;
            fn_decl->name.lexeme = mangled_name;

            visit_function_decl(fn_decl);

            fn_decl->name = original_token;
        }
    }
}

void IRGenerator::collect_all_structs(Program *program) {
    for (const auto &item : program->items) {
        if (auto struct_decl = dynamic_cast<StructDecl *>(item.get())) {
            local_structs_set_.insert(struct_decl);
        } else if (auto fn_decl = dynamic_cast<FnDecl *>(item.get())) {
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
        for (const auto &s : block_stmt->statements) {
            collect_structs_from_stmt(s.get());
        }
    } else if (auto item_stmt = dynamic_cast<ItemStmt *>(stmt)) {
        if (item_stmt->item) {
            if (auto struct_decl = dynamic_cast<StructDecl *>(item_stmt->item.get())) {
                local_structs_set_.insert(struct_decl);
            } else if (auto fn_decl = dynamic_cast<FnDecl *>(item_stmt->item.get())) {
                if (fn_decl->body.has_value() && fn_decl->body.value()) {
                    collect_structs_from_stmt(fn_decl->body.value().get());
                }
            }
        }
    }
}
