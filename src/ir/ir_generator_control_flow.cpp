/**
 * @file ir_generator_control_flow.cpp
 * @brief IR生成器 - 控制流表达式处理模块
 *
 * 本文件包含控制流Expression节点的IR生成实现：
 * - IfExpr: if表达式（包括if-else，phi节点处理）
 * - WhileExpr: while循环
 * - LoopExpr: 无限循环
 *
 * 控制流特性：
 * - 基本块管理和跳转
 * - 条件分支和phi节点
 * - break/continue支持
 * - 终止标志管理
 */

#include "ir_generator.h"

// ========== 控制流表达式 Visitors ==========

void IRGenerator::visit(IfExpr *node) {
    // if 表达式生成 IR
    //
    // 生成结构:
    // 当前块:
    //   计算条件
    //   br i1 %cond, label %if.then.N, label %if.else.N (或 %if.end.N)
    // if.then.N:
    //   then 分支代码
    //   br label %if.end.N
    // if.else.N: (如果有 else)
    //   else 分支代码
    //   br label %if.end.N
    // if.end.N:
    //   如果有返回值: %result = phi i32 [%then_val, %if.then.N], [%else_val, %if.else.N]

    int current_if = if_counter_++;

    // 标签名称
    std::string then_label = "if.then." + std::to_string(current_if);
    std::string else_label = "if.else." + std::to_string(current_if);
    std::string end_label = "if.end." + std::to_string(current_if);

    // 1. 计算条件表达式
    node->condition->accept(this);
    std::string cond_var = get_expr_result(node->condition.get());

    // 2. 生成条件分支
    if (node->else_branch.has_value()) {
        // 有 else 分支: br i1 %cond, label %then, label %else
        emitter_.emit_cond_br(cond_var, then_label, else_label);
    } else {
        // 无 else 分支: br i1 %cond, label %then, label %end
        emitter_.emit_cond_br(cond_var, then_label, end_label);
    }

    // 3. 生成 then 分支
    begin_block(then_label);
    current_block_terminated_ = false; // 重置终止标志

    node->then_branch->accept(this);
    std::string then_result;
    bool then_has_value = false;
    bool then_terminated = current_block_terminated_; // 记录 then 是否终止

    // 检查 then 分支是否有返回值
    if (node->type && node->type->kind != TypeKind::UNIT) {
        then_result = get_expr_result(node->then_branch.get());
        then_has_value = true;
    }

    // 记录then分支结束时的实际块标签（用于PHI节点）
    std::string then_pred_block = current_block_label_;
    // std::cerr << "DEBUG: then_pred_block = " << then_pred_block << ", then_label = " <<
    // then_label << std::endl;

    // then 分支跳转到 end（除非已经有 return/break/continue）
    if (!then_terminated) {
        emitter_.emit_br(end_label);
    }

    // 4. 生成 else 分支（如果有）
    std::string else_result;
    bool else_has_value = false;
    bool else_terminated = false;
    std::string else_pred_block; // 记录else分支结束时的实际块标签

    if (node->else_branch.has_value()) {
        begin_block(else_label);
        current_block_terminated_ = false; // 重置终止标志

        node->else_branch.value()->accept(this);
        else_terminated = current_block_terminated_; // 记录 else 是否终止

        // 检查 else 分支是否有返回值
        if (node->type && node->type->kind != TypeKind::UNIT) {
            else_result = get_expr_result(node->else_branch.value().get());
            else_has_value = true;
        }

        // 记录else分支结束时的实际块标签（用于PHI节点）
        else_pred_block = current_block_label_;

        // else 分支跳转到 end
        if (!else_terminated) {
            emitter_.emit_br(end_label);
        }
    }

    // 5. 生成 end 基本块（只有在至少一个分支会跳转到这里时才生成）
    bool need_end_block = !then_terminated || (node->else_branch.has_value() && !else_terminated) ||
                          !node->else_branch.has_value();

    if (need_end_block) {
        begin_block(end_label);
        current_block_terminated_ = false; // 重置终止标志
    }

    // 5. 生成 PHI 节点合并结果（如果两个分支都有值且类型不是unit）
    bool is_unit_type = (node->type->kind == TypeKind::UNIT);

    if (need_end_block && then_has_value && else_has_value && !is_unit_type &&
        (!then_terminated || !else_terminated)) {
        std::string result_type = type_mapper_.map(node->type.get());

        // 对于聚合类型（数组/结构体），PHI节点应该使用指针类型
        bool is_aggregate =
            (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);
        if (is_aggregate) {
            result_type += "*"; // 改为指针类型
        }

        std::vector<std::pair<std::string, std::string>> phi_incoming;

        // 只添加未终止且结果非空的分支，使用记录的前驱块标签
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
        // 只有 then 分支有值且未终止且不是unit类型
        store_expr_result(node, then_result);
    } else if (else_has_value && !else_terminated && !is_unit_type) {
        // 只有 else 分支有值且未终止且不是unit类型
        store_expr_result(node, else_result);
    } else {
        // if 表达式没有返回值（类型为 unit）或者所有分支都终止了
        store_expr_result(node, ""); // 空字符串表示 unit 类型
    }
}

void IRGenerator::visit(WhileExpr *node) {
    // while 循环实现：
    // while (condition) { body }
    //
    // 生成的 IR 结构:
    // br label %while.cond.N
    // while.cond.N:
    //   condition evaluation
    //   br i1 %cond, label %while.body.N, label %while.end.N
    // while.body.N:
    //   body
    //   br label %while.cond.N
    // while.end.N:
    //   (继续执行)

    int current_while = while_counter_++;

    std::string cond_label = "while.cond." + std::to_string(current_while);
    std::string body_label = "while.body." + std::to_string(current_while);
    std::string end_label = "while.end." + std::to_string(current_while);

    // 将循环上下文推入栈（用于 break/continue）
    loop_stack_.push_back({cond_label, end_label});

    // 1. 跳转到条件检查
    emitter_.emit_br(cond_label);

    // 2. 条件检查块
    begin_block(cond_label);
    node->condition->accept(this);
    std::string cond_var = get_expr_result(node->condition.get());

    emitter_.emit_cond_br(cond_var, body_label, end_label);

    // 3. 循环体块
    begin_block(body_label);
    current_block_terminated_ = false; // 重置终止标志

    if (node->body) {
        node->body->accept(this);
    }

    // 跳回条件检查（除非有 break/continue/return）
    if (!current_block_terminated_) {
        emitter_.emit_br(cond_label);
    }

    // 4. 结束块
    begin_block(end_label);
    current_block_terminated_ = false; // 重置终止标志

    // 弹出循环上下文
    loop_stack_.pop_back();

    // while 表达式本身不产生值（返回 unit）
    store_expr_result(node, "");
}

void IRGenerator::visit(LoopExpr *node) {
    // loop 循环实现：
    // loop { body }
    //
    // 生成的 IR 结构:
    // br label %loop.body.N
    // loop.body.N:
    //   body
    //   br label %loop.body.N
    // loop.end.N:
    //   (break 跳转到这里)

    int current_loop = loop_counter_++;

    std::string body_label = "loop.body." + std::to_string(current_loop);
    std::string end_label = "loop.end." + std::to_string(current_loop);

    // 将循环上下文推入栈（用于 break/continue）
    loop_stack_.push_back({body_label, end_label});

    // 1. 跳转到循环体
    emitter_.emit_br(body_label);

    // 2. 循环体块
    begin_block(body_label);
    current_block_terminated_ = false; // 重置终止标志

    if (node->body) {
        node->body->accept(this);
    }

    // 无条件跳回循环体开始（除非有 break/return）
    if (!current_block_terminated_) {
        emitter_.emit_br(body_label);
    }

    // 3. 结束块（只能通过 break 到达）
    begin_block(end_label);
    current_block_terminated_ = false; // 重置终止标志

    // 弹出循环上下文
    loop_stack_.pop_back();

    // loop 表达式本身不产生值（返回 unit）
    store_expr_result(node, "");
}
