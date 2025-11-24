#include "ir_generator.h"

#include <map>
#include <set>

/**
 * Generate IR for block statements.
 *
 * Features:
 * - Creates new variable scope (enter/exit scope)
 * - Executes statements sequentially
 * - Handles optional final expression (block value)
 * - Respects early termination (return/break/continue)
 *
 * Scope management:
 * - Variables defined in block are destroyed on exit
 * - Supports variable shadowing
 * - Nested blocks create nested scopes
 *
 * @param node The block statement AST node
 */
void IRGenerator::visit(BlockStmt *node) {
    value_manager_.enter_scope();

    for (size_t i = 0; i < node->statements.size(); ++i) {
        if (current_block_terminated_) {
            break;
        }

        auto &stmt = node->statements[i];
        stmt->accept(this);
    }

    if (node->final_expr.has_value()) {
        auto final_expr = node->final_expr.value();
        if (final_expr) {
            final_expr->accept(this);
        }
    }

    value_manager_.exit_scope();
}

/**
 * Generate IR for expression statements.
 *
 * Purpose: Execute expression for its side effects, discard result
 *
 * Examples:
 * - Function calls: print(x);
 * - Method calls: vec.push(item);
 * - Assignments: x = 5; (though assignment is actually an expression)
 *
 * Process:
 * 1. Evaluate the expression
 * 2. Ignore the returned value
 * 3. Only side effects matter (function calls, assignments, etc.)
 *
 * Note:
 * - Pure expressions without side effects are wasteful but legal
 * - Examples: 42; or x + y; (computed but unused)
 * - Semantic analyzer may warn about unused results
 *
 * @param node The expression statement AST node
 */
void IRGenerator::visit(ExprStmt *node) {
    if (node->expression) {
        node->expression->accept(this);
    }
}

/**
 * Generate IR for let statements (variable declarations).
 *
 * Syntax: let [mut] name: Type = initializer;
 *
 * Type inference:
 * - Explicit type annotation takes priority
 * - Otherwise inferred from initializer expression
 *
 * Handling strategies:
 * 1. Reference types (&T, &mut T):
 *    - No alloca needed, store pointer directly
 *    - Pointer value comes from initializer
 *
 * 2. Aggregate types returning pointer (array/struct literals, function calls):
 *    - Initializer already returns pointer
 *    - Use that pointer directly, no copy needed
 *
 * 3. Regular types:
 *    - Allocate stack space (alloca)
 *    - Evaluate initializer
 *    - Store value to allocated space
 *
 * @param node The let statement AST node
 */
void IRGenerator::visit(LetStmt *node) {
    auto id_pattern = dynamic_cast<IdentifierPattern *>(node->pattern.get());
    if (!id_pattern) {
        return;
    }

    std::string var_name = id_pattern->name.lexeme;
    bool is_mutable = id_pattern->is_mutable;

    std::shared_ptr<Type> var_type;

    if (node->type_annotation.has_value() && node->type_annotation.value()->resolved_type) {
        var_type = node->type_annotation.value()->resolved_type;
    } else if (node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        if (init_expr && init_expr->type) {
            var_type = init_expr->type;
        }
    }

    if (!var_type) {
        return;
    }

    std::string type_str = type_mapper_.map(var_type.get());
    bool is_reference = (var_type->kind == TypeKind::REFERENCE);
    bool is_aggregate_returns_pointer = false;
    std::string alloca_name;

    if (node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        if (init_expr) {
            bool is_literal = (dynamic_cast<ArrayLiteralExpr *>(init_expr.get()) ||
                               dynamic_cast<ArrayInitializerExpr *>(init_expr.get()) ||
                               dynamic_cast<StructInitializerExpr *>(init_expr.get()));

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

    if (is_reference && node->initializer.has_value()) {
        auto init_expr = node->initializer.value();
        init_expr->accept(this);
        alloca_name = get_expr_result(init_expr.get());

        value_manager_.define_variable(var_name, alloca_name, type_str, is_mutable);
        return;
    }

    if (is_aggregate_returns_pointer) {
        auto init_expr = node->initializer.value();
        init_expr->accept(this);
        alloca_name = get_expr_result(init_expr.get());
    } else {
        alloca_name = emitter_.emit_alloca(type_str);

        if (node->initializer.has_value()) {
            auto init_expr = node->initializer.value();
            if (init_expr) {
                bool is_aggregate =
                    (var_type->kind == TypeKind::ARRAY || var_type->kind == TypeKind::STRUCT);

                if (is_aggregate) {
                    set_target_address(alloca_name);
                }

                init_expr->accept(this);
                take_target_address();

                std::string init_value = get_expr_result(init_expr.get());
                if (!init_value.empty()) {
                    if (is_aggregate && init_value == alloca_name) {
                    } else if (is_aggregate) {
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

    std::string ptr_type_str = type_str + "*";
    value_manager_.define_variable(var_name, alloca_name, ptr_type_str, is_mutable);
}

/**
 * Generate IR for return statements.
 *
 * Forms:
 * 1. return; (void return)
 *    -> ret void
 *
 * 2. return value; (scalar return)
 *    -> ret i32 %value
 *
 * 3. return struct; (SRET optimization)
 *    -> memcpy to sret pointer, then ret void
 *
 * SRET handling:
 * - Large structs use hidden pointer parameter
 * - Function signature: void @foo(%Struct* sret %retval)
 * - Copy result to sret pointer, return void
 *
 * Example SRET:
 *   fn get_point() -> Point {
 *       let p = Point { x: 1, y: 2 };
 *       return p;  // Copies to sret pointer
 *   }
 *
 * Example scalar:
 *   fn add(a: i32, b: i32) -> i32 {
 *       return a + b;  // ret i32 %sum
 *   }
 *
 * @param node The return statement AST node
 */
void IRGenerator::visit(ReturnStmt *node) {
    if (node->value.has_value()) {
        auto return_expr = node->value.value();
        if (return_expr) {
            return_expr->accept(this);

            std::string return_value = get_expr_result(return_expr.get());

            if (return_expr->type) {
                std::string expr_type_str = type_mapper_.map(return_expr->type.get());

                if (current_function_uses_sret_) {
                    emitter_.emit_ret_void();
                } else {
                    bool is_aggregate = (return_expr->type->kind == TypeKind::ARRAY ||
                                         return_expr->type->kind == TypeKind::STRUCT);

                    if (is_aggregate) {
                        return_value = emitter_.emit_load(expr_type_str, return_value);
                    }

                    if (!current_function_return_type_str_.empty() &&
                        expr_type_str != current_function_return_type_str_) {
                        if ((expr_type_str == "i32" || expr_type_str == "i64") &&
                            (current_function_return_type_str_ == "i32" ||
                             current_function_return_type_str_ == "i64")) {
                            if (expr_type_str == "i32" &&
                                current_function_return_type_str_ == "i64") {
                                return_value = emitter_.emit_sext(return_value, "i32", "i64");
                            } else if (expr_type_str == "i64" &&
                                       current_function_return_type_str_ == "i32") {
                                return_value = emitter_.emit_trunc(return_value, "i64", "i32");
                            }
                        }
                    }

                    std::string ret_type = current_function_return_type_str_.empty()
                                               ? expr_type_str
                                               : current_function_return_type_str_;
                    emitter_.emit_ret(ret_type, return_value);
                }
                current_block_terminated_ = true;
            }
        }
    } else {
        emitter_.emit_ret_void();
        current_block_terminated_ = true;
    }
}

void IRGenerator::visit(BreakStmt *node) {
    if (loop_stack_.empty()) {
        return;
    }

    std::string break_label = loop_stack_.back().break_label;

    if (node->value.has_value()) {
    }

    emitter_.emit_br(break_label);
    current_block_terminated_ = true;
}

/**
 * Generate IR for continue statements.
 *
 * Purpose: Skip rest of loop body, jump to next iteration
 *
 * Process:
 * 1. Look up continue target from continue_stack_
 *    - For while: jumps to condition check (while.cond)
 *    - For loop: jumps to loop body start (loop.body)
 * 2. Generate unconditional branch to that label
 * 3. Mark current block as terminated
 *
 * IR examples:
 * - While: br label %while.cond.N
 * - Loop: br label %loop.body.N
 *
 * Context requirements:
 * - Must be inside a loop (continue_stack_ not empty)
 * - Semantic analysis ensures this
 *
 * @param node The continue statement AST node
 * @note Code after continue in same block is unreachable
 */
void IRGenerator::visit(ContinueStmt *node) {
    if (loop_stack_.empty()) {
        return;
    }

    std::string continue_label = loop_stack_.back().continue_label;

    emitter_.emit_br(continue_label);
    current_block_terminated_ = true;
}

/**
 * Generate IR for item statements (struct declarations inside functions).
 *
 * Rust allows struct definitions inside function bodies:
 *   fn main() {
 *       struct Point { x: i32, y: i32 }
 *       let p = Point { x: 1, y: 2 };
 *   }
 *
 * Handling:
 * - Struct types must be global in LLVM IR
 * - Local struct definitions are "hoisted" to module level
 * - Actually processed by collect_all_structs() before function generation
 * - This function is essentially a no-op in IR generation phase
 *
 * Why this works:
 * 1. First pass: collect_all_structs() finds all structs
 * 2. Emit all struct type definitions globally
 * 3. Second pass: generate functions
 * 4. When visiting ItemStmt, type already defined
 *
 * @param node The item statement AST node
 * @note Type definition already emitted, no IR generation needed here
 */
void IRGenerator::visit(ItemStmt *node) {

    if (!node->item) {
        return;
    }

    if (auto fn_decl = dynamic_cast<FnDecl *>(node->item.get())) {
        if (inside_function_body_) {
            nested_functions_.push_back(fn_decl);
        } else {
            visit_function_decl(fn_decl);
        }
        return;
    }

    if (auto struct_decl = dynamic_cast<StructDecl *>(node->item.get())) {
        return;
    }

    if (auto const_decl = dynamic_cast<ConstDecl *>(node->item.get())) {
        if (!const_decl->type || !const_decl->type->resolved_type) {
            return;
        }

        std::string llvm_type = type_mapper_.map(const_decl->type->resolved_type.get());
        std::string const_name = const_decl->name.lexeme;

        std::string alloca_ptr = emitter_.emit_alloca(llvm_type);

        const_decl->value->accept(this);
        std::string init_value = get_expr_result(const_decl->value.get());

        if (init_value.empty()) {
            return;
        }

        emitter_.emit_store(llvm_type, init_value, alloca_ptr);

        value_manager_.define_variable(const_name, alloca_ptr, llvm_type + "*", false);
    }
}
