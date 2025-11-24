
#include "ir_generator.h"

/**
 * Generate IR for if expressions with optional else branch.
 *
 * IR structure:
 *   current_block:
 *     %cond = <condition evaluation>
 *     br i1 %cond, label %if.then.N, label %if.else.N (or %if.end.N)
 *
 *   if.then.N:
 *     <then branch code>
 *     br label %if.end.N
 *
 *   if.else.N:  (if else exists)
 *     <else branch code>
 *     br label %if.end.N
 *
 *   if.end.N:
 *     %result = phi T [%then_val, %actual_then_block], [%else_val, %actual_else_block]
 *
 * Features:
 * - PHI nodes for merging values from different branches
 * - Proper handling of early termination (return/break/continue)
 * - Support for if-expressions (returns value) and if-statements (unit type)
 * - Tracks actual predecessor blocks for accurate PHI generation
 *
 * @param node The if expression AST node
 */
void IRGenerator::visit(IfExpr *node) {
    int current_if = if_counter_++;

    std::string then_label = "if.then." + std::to_string(current_if);
    std::string else_label = "if.else." + std::to_string(current_if);
    std::string end_label = "if.end." + std::to_string(current_if);

    node->condition->accept(this);
    std::string cond_var = get_expr_result(node->condition.get());

    if (node->else_branch.has_value()) {
        emitter_.emit_cond_br(cond_var, then_label, else_label);
    } else {
        emitter_.emit_cond_br(cond_var, then_label, end_label);
    }

    begin_block(then_label);
    current_block_terminated_ = false;

    node->then_branch->accept(this);
    std::string then_result;
    bool then_has_value = false;
    bool then_terminated = current_block_terminated_;

    if (node->type && node->type->kind != TypeKind::UNIT) {
        then_result = get_expr_result(node->then_branch.get());
        then_has_value = true;
    }

    std::string then_pred_block = current_block_label_;

    if (!then_terminated) {
        emitter_.emit_br(end_label);
    }

    std::string else_result;
    bool else_has_value = false;
    bool else_terminated = false;
    std::string else_pred_block;

    if (node->else_branch.has_value()) {
        begin_block(else_label);
        current_block_terminated_ = false;

        node->else_branch.value()->accept(this);
        else_terminated = current_block_terminated_;

        if (node->type && node->type->kind != TypeKind::UNIT) {
            else_result = get_expr_result(node->else_branch.value().get());
            else_has_value = true;
        }

        else_pred_block = current_block_label_;

        if (!else_terminated) {
            emitter_.emit_br(end_label);
        }
    }

    bool need_end_block = !then_terminated || (node->else_branch.has_value() && !else_terminated) ||
                          !node->else_branch.has_value();

    if (need_end_block) {
        begin_block(end_label);
        current_block_terminated_ = false;
    }

    bool is_unit_type = (node->type->kind == TypeKind::UNIT);

    if (need_end_block && then_has_value && else_has_value && !is_unit_type &&
        (!then_terminated || !else_terminated)) {
        std::string result_type = type_mapper_.map(node->type.get());

        bool is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
        if (is_aggregate) {
            result_type += "*";
        }

        std::vector<std::pair<std::string, std::string>> phi_incoming;

        if (!then_terminated && !then_result.empty()) {
            phi_incoming.push_back({then_result, then_pred_block});
        }
        if (!else_terminated && !else_result.empty()) {
            phi_incoming.push_back({else_result, else_pred_block});
        }

        if (!phi_incoming.empty()) {
            std::string phi_result = emitter_.emit_phi(result_type, phi_incoming);
            store_expr_result(node, phi_result);
        }
    } else if (then_has_value && !then_terminated && !is_unit_type) {
        store_expr_result(node, then_result);
    } else if (else_has_value && !else_terminated && !is_unit_type) {
        store_expr_result(node, else_result);
    } else {
        store_expr_result(node, "");
    }
}

/**
 * Generate IR for while loop expressions.
 *
 * Syntax: while condition { body }
 *
 * IR structure:
 *   br label %while.cond.N
 *
 * while.cond.N:
 *   %cond = <evaluate condition>
 *   br i1 %cond, label %while.body.N, label %while.end.N
 *
 * while.body.N:
 *   <loop body statements>
 *   br label %while.cond.N
 *
 * while.end.N:
 *   ; Continue after loop
 *
 * Loop control:
 * - break: Jumps to %while.end.N
 * - continue: Jumps to %while.cond.N (re-evaluate condition)
 *
 * Value:
 * - While loops always return () (unit type)
 * - Not expressions in terms of producing values
 *
 * Example:
 *   while i < 10 {
 *       print(i);
 *       i += 1;
 *   }
 *
 * @param node The while expression AST node
 */
void IRGenerator::visit(WhileExpr *node) {

    int current_while = while_counter_++;

    std::string cond_label = "while.cond." + std::to_string(current_while);
    std::string body_label = "while.body." + std::to_string(current_while);
    std::string end_label = "while.end." + std::to_string(current_while);

    loop_stack_.push_back({cond_label, end_label});

    emitter_.emit_br(cond_label);

    begin_block(cond_label);
    node->condition->accept(this);
    std::string cond_var = get_expr_result(node->condition.get());

    emitter_.emit_cond_br(cond_var, body_label, end_label);

    begin_block(body_label);
    current_block_terminated_ = false;

    if (node->body) {
        node->body->accept(this);
    }

    if (!current_block_terminated_) {
        emitter_.emit_br(cond_label);
    }

    begin_block(end_label);
    current_block_terminated_ = false;

    loop_stack_.pop_back();

    store_expr_result(node, "");
}

/**
 * Generate IR for infinite loop expressions.
 *
 * Syntax: loop { body }
 *
 * IR structure:
 *   br label %loop.body.N
 *
 * loop.body.N:
 *   <loop body statements>
 *   br label %loop.body.N  ; Unconditional jump back
 *
 * loop.end.N:
 *   ; Only reached via break
 *
 * Exiting:
 * - Must use 'break' to exit (no condition)
 * - Without break, infinite loop
 * - break with value: Becomes loop's return value
 *
 * Loop control:
 * - break: Jumps to %loop.end.N
 * - continue: Jumps to %loop.body.N (restart iteration)
 *
 * Example:
 *   let result = loop {
 *       counter += 1;
 *       if counter > 10 {
 *           break counter;  // Returns counter value
 *       }
 *   };
 *
 * @param node The loop expression AST node
 */
void IRGenerator::visit(LoopExpr *node) {

    int current_loop = loop_counter_++;

    std::string body_label = "loop.body." + std::to_string(current_loop);
    std::string end_label = "loop.end." + std::to_string(current_loop);

    loop_stack_.push_back({body_label, end_label});

    emitter_.emit_br(body_label);

    begin_block(body_label);
    current_block_terminated_ = false;

    if (node->body) {
        node->body->accept(this);
    }

    if (!current_block_terminated_) {
        emitter_.emit_br(body_label);
    }

    begin_block(end_label);
    current_block_terminated_ = false;

    loop_stack_.pop_back();

    store_expr_result(node, "");
}
