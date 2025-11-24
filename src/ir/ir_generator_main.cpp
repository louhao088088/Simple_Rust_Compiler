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

/**
 * Dispatch to appropriate visitor based on item type.
 *
 * Top-level items:
 * - fn: Function declarations -> visit_function_decl()
 * - struct: Type definitions -> (handled in first pass)
 * - const: Global constants -> visit_const_decl()
 * - impl: Method implementations -> visit_impl_block()
 *
 * Processing order:
 * 1. First pass: collect_all_structs() gathers struct definitions
 * 2. Second pass: This function processes functions, consts, impls
 *
 * Why struct_decl is empty:
 * - Struct types already emitted before functions
 * - No additional IR generation needed here
 *
 * Example program:
 *   struct Point { x: i32, y: i32 }  // Skipped (already emitted)
 *   const MAX: i32 = 100;             // visit_const_decl()
 *   fn main() { ... }                 // visit_function_decl()
 *   impl Point { ... }                // visit_impl_block()
 *
 * @param item The top-level item to process
 */
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

/**
 * Generate IR for struct type declarations.
 *
 * Example Rust:
 *   struct Point {
 *       x: i32,
 *       y: i32,
 *   }
 *
 * Generated LLVM IR:
 *   %Point = type { i32, i32 }
 *
 * Process:
 * 1. Extract struct type information from semantic analysis
 * 2. Map each field to LLVM type string
 * 3. Preserve field order (important for GEP indices)
 * 4. Emit type definition to IR
 *
 * Field ordering:
 * - Follows declaration order in source code
 * - GEP uses numeric indices: 0, 1, 2...
 * - Example: point.y -> getelementptr %Point, ..., i32 1
 *
 * Type handling:
 * - Recursive structs use pointers: struct Node { next: &Node }
 * - Nested structs are inlined: struct Outer { inner: Inner }
 * - Arrays in structs: struct Data { values: [i32; 10] }
 *
 * @param node The struct declaration AST node
 */
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

/**
 * Generate IR for const declarations.
 *
 * Example Rust:
 *   const PI: f64 = 3.14159;
 *   const MAX_SIZE: i32 = 1000;
 *
 * Generated LLVM IR:
 *   @PI = constant double 3.14159
 *   @MAX_SIZE = constant i32 1000
 *
 * Process:
 * 1. Evaluate constant expression at compile time
 * 2. Emit as global constant (not variable)
 * 3. Store value for later constant folding
 *
 * Constant evaluation:
 * - Literals: Direct value extraction
 * - Arithmetic: Compile-time computation
 * - String literals: Global constant array
 * - Const references: Look up in const_values_ map
 *
 * Differences from let:
 * - Globals vs stack allocation
 * - Immutable by nature (not just semantic)
 * - Must be compile-time evaluable
 * - No address taking (inlined at use sites)
 *
 * @param node The const declaration AST node
 */
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

/**
 * Generate IR for impl blocks (method implementations).
 *
 * Example Rust:
 *   impl Point {
 *       fn new(x: i32, y: i32) -> Point { ... }
 *       fn distance(&self) -> i32 { ... }
 *   }
 *
 * Process:
 * - Each method is generated as a regular function
 * - First parameter of instance methods is 'self' (implicit pointer)
 * - Static methods (no self) are just regular functions
 *
 * Method name mangling:
 * - new -> Point_new
 * - distance -> Point_distance
 * - Prevents name collision between types
 *
 * Self parameter:
 * - &self -> %self: T*
 * - &mut self -> %self: T* (mutability is semantic only)
 * - self -> %self: T (moves ownership, rare)
 *
 * @param node The impl block AST node
 */
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

/**
 * Collect all struct declarations from the entire program.
 *
 * Purpose: Ensure all struct types are defined before functions
 *
 * Process:
 * 1. Traverse top-level items for struct declarations
 * 2. Recursively search function bodies for local structs
 * 3. Store unique structs in local_structs_set_
 * 4. Emit all collected struct definitions
 *
 * Why needed:
 * - LLVM requires type definitions before use
 * - Functions may reference structs defined inside other functions
 * - Local structs (in function bodies) must be hoisted to global scope
 *
 * Example:
 *   fn main() {
 *       struct Point { x: i32, y: i32 }  // Local struct
 *       let p = Point { x: 1, y: 2 };
 *   }
 *
 * All struct types must be emitted at module level before any function.
 *
 * @param program The program AST root node
 * @note Prevents "use of undefined type" errors in IR
 */
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

/**
 * Recursively collect struct declarations from statement trees.
 *
 * Purpose:
 * - Find structs defined inside function bodies
 * - Add them to local_structs_set_ for later processing
 * - Enables struct hoisting to module level
 *
 * Example:
 *   fn main() {
 *       struct Point { x: i32, y: i32 }  // Found here
 *       let p = Point { x: 1, y: 2 };
 *       {
 *           struct Color { r: u8, g: u8 }  // Found in nested block
 *       }
 *   }
 *
 * Traversal:
 * - BlockStmt: Recursively check all statements
 * - ItemStmt: Extract struct/function declarations
 * - Other statements: Ignored (no item declarations)
 *
 * Why this is needed:
 * - LLVM requires type definitions before use
 * - Rust allows structs anywhere in function body
 * - Must collect and emit all struct types first
 * - Then generate function bodies that use them
 *
 * @param stmt The statement to search for struct declarations
 */
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
