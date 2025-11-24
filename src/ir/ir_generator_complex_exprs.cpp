#include "ir_generator.h"

/**
 * Generate IR for array literal expressions.
 *
 * Example: [1, 2, 3, 4, 5]
 *
 * Strategy:
 * 1. Allocate array on stack (or use target_address if provided)
 * 2. For each element:
 *    a. Generate element expression
 *    b. Use GEP to calculate element pointer
 *    c. Store element value
 *
 * Optimization:
 * - Target address mechanism: allows in-place initialization
 * - Avoids extra copy when used in let statement or return
 *
 * @param node The array literal expression AST node
 * @return Pointer to the initialized array
 */
void IRGenerator::visit(ArrayLiteralExpr *node) {

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->type);
    if (!array_type) {
        store_expr_result(node, "");
        return;
    }

    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    size_t array_size = node->elements.size();
    std::string array_ir_type = "[" + std::to_string(array_size) + " x " + elem_ir_type + "]";

    std::string array_ptr = take_target_address();
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    for (size_t i = 0; i < node->elements.size(); ++i) {
        node->elements[i]->accept(this);
        std::string elem_value = get_expr_result(node->elements[i].get());

        if (elem_value.empty()) {
            continue;
        }

        bool elem_is_aggregate = false;
        if (auto elem_type = node->elements[i]->type) {
            elem_is_aggregate =
                (elem_type->kind == TypeKind::ARRAY || elem_type->kind == TypeKind::STRUCT);
        }

        if (elem_is_aggregate) {
            elem_value = emitter_.emit_load(elem_ir_type, elem_value);
        }

        std::vector<std::string> indices = {"i64 0", "i64 " + std::to_string(i)};
        std::string elem_ptr =
            emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

        emitter_.emit_store(elem_ir_type, elem_value, elem_ptr);
    }

    store_expr_result(node, array_ptr);
}

/**
 * Generate IR for array initializer expressions.
 *
 * Syntax: [value; size]  // Creates array with 'size' copies of 'value'
 * Example: [0; 100]  // Array of 100 zeros
 *
 * Optimization strategies:
 * 1. Small arrays (≤16 elements):
 *    - Unroll as individual GEP + store instructions
 *    - Best for small constant-size arrays
 *
 * 2. Zero initialization (>64 elements):
 *    - Use llvm.memset.p0i8 intrinsic
 *    - Single call to set all bytes to zero
 *    - Most efficient for zero-init
 *
 * 3. Large arrays with non-zero value (>16 elements):
 *    - Generate initialization loop:
 *      for (i = 0; i < size; i++) arr[i] = value;
 *    - Avoids code bloat from unrolling
 *
 * @param node The array initializer expression AST node
 * @return Pointer to the initialized array
 */
void IRGenerator::visit(ArrayInitializerExpr *node) {

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->type);
    if (!array_type) {
        store_expr_result(node, "");
        return;
    }

    std::string target_ptr = take_target_address();

    size_t array_size = array_type->size;

    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    std::string array_ir_type = "[" + std::to_string(array_size) + " x " + elem_ir_type + "]";

    bool value_is_aggregate = false;
    if (node->value->type) {
        value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                              node->value->type->kind == TypeKind::STRUCT);
    }

    if (value_is_aggregate) {
        std::string temp_ptr = emitter_.emit_alloca(elem_ir_type);
        set_target_address(temp_ptr);
    }

    node->value->accept(this);

    take_target_address();

    std::string init_value = get_expr_result(node->value.get());

    if (init_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    if (value_is_aggregate) {
        init_value = emitter_.emit_load(elem_ir_type, init_value);
    }

    std::string array_ptr = target_ptr;
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    const size_t UNROLL_THRESHOLD = 16;
    const size_t MEMSET_THRESHOLD = 64;

    bool is_zero_init = is_zero_initializer(node->value.get());

    size_t elem_size = 0;
    if (elem_ir_type == "i1")
        elem_size = 1;
    else if (elem_ir_type == "i8")
        elem_size = 1;
    else if (elem_ir_type == "i32")
        elem_size = 4;
    else if (elem_ir_type == "i64")
        elem_size = 8;
    else if (elem_ir_type.find('%') == 0) {
        elem_size = get_type_size(array_type->element_type.get());
    }

    if (is_zero_init && array_size > MEMSET_THRESHOLD && elem_size > 0) {
        size_t total_bytes = array_size * elem_size;

        emitter_.emit_memset(array_ptr, 0, total_bytes, array_ir_type + "*");
    } else if (array_size <= UNROLL_THRESHOLD) {
        for (size_t i = 0; i < array_size; ++i) {
            std::vector<std::string> indices = {"i64 0", "i64 " + std::to_string(i)};
            std::string elem_ptr =
                emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

            emitter_.emit_store(elem_ir_type, init_value, elem_ptr);
        }
    } else {

        std::string loop_cond_label = emitter_.new_label();
        std::string loop_body_label = emitter_.new_label();
        std::string loop_end_label = emitter_.new_label();

        std::string i_ptr = emitter_.emit_alloca("i64");
        emitter_.emit_store("i64", "0", i_ptr);
        emitter_.emit_br(loop_cond_label);

        begin_block(loop_cond_label);
        std::string i_val = emitter_.emit_load("i64", i_ptr);
        std::string cmp = emitter_.emit_icmp("slt", "i64", i_val, std::to_string(array_size));
        emitter_.emit_cond_br(cmp, loop_body_label, loop_end_label);

        begin_block(loop_body_label);
        std::vector<std::string> indices = {"i64 0", "i64 " + i_val};
        std::string elem_ptr =
            emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);
        emitter_.emit_store(elem_ir_type, init_value, elem_ptr);

        std::string next_i = emitter_.emit_binary_op("add", "i64", i_val, "1");
        emitter_.emit_store("i64", next_i, i_ptr);
        emitter_.emit_br(loop_cond_label);

        begin_block(loop_end_label);
    }

    store_expr_result(node, array_ptr);
}

void IRGenerator::visit(IndexExpr *node) {

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true;
    node->object->accept(this);
    generating_lvalue_ = was_generating_lvalue;

    std::string array_ptr = get_expr_result(node->object.get());

    if (array_ptr.empty()) {
        store_expr_result(node, "");
        return;
    }

    generating_lvalue_ = false;
    node->index->accept(this);
    generating_lvalue_ = was_generating_lvalue;

    std::string index_value = get_expr_result(node->index.get());

    if (index_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    std::string index_type_str = type_mapper_.map(node->index->type.get());
    if (index_type_str == "i32") {
        bool is_signed = (node->index->type->kind == TypeKind::ISIZE ||
                          node->index->type->kind == TypeKind::I32);
        if (is_signed) {
            index_value = emitter_.emit_sext("i32", index_value, "i64");
        } else {
            index_value = emitter_.emit_zext("i32", index_value, "i64");
        }
    }

    std::shared_ptr<Type> actual_type = node->object->type;
    if (actual_type->kind == TypeKind::REFERENCE) {
        auto ref_type = std::dynamic_pointer_cast<ReferenceType>(actual_type);
        if (ref_type) {
            actual_type = ref_type->referenced_type;
        }
    }

    auto array_type = std::dynamic_pointer_cast<ArrayType>(actual_type);
    if (!array_type) {
        store_expr_result(node, "");
        return;
    }

    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    std::string array_ir_type = "[" + std::to_string(array_type->size) + " x " + elem_ir_type + "]";

    std::vector<std::string> indices = {"i64 0", "i64 " + index_value};
    std::string elem_ptr = emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

    bool elem_is_aggregate = (array_type->element_type->kind == TypeKind::ARRAY ||
                              array_type->element_type->kind == TypeKind::STRUCT);

    if (generating_lvalue_ || elem_is_aggregate) {
        store_expr_result(node, elem_ptr);
    } else {
        std::string elem_value = emitter_.emit_load(elem_ir_type, elem_ptr);
        store_expr_result(node, elem_value);
    }
}

// Generate IR for struct initializer expressions.
void IRGenerator::visit(StructInitializerExpr *node) {

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    auto struct_type = std::dynamic_pointer_cast<StructType>(node->type);
    if (!struct_type) {
        store_expr_result(node, "");
        return;
    }

    std::string struct_ir_type = type_mapper_.map(struct_type.get());

    std::string struct_ptr;

    std::string target_ptr = take_target_address();

    auto sret_self = value_manager_.lookup_variable("__sret_self");
    if (!target_ptr.empty()) {
        struct_ptr = target_ptr;
    } else if (sret_self) {
        struct_ptr = sret_self->alloca_name;
    } else {
        struct_ptr = emitter_.emit_alloca(struct_ir_type);
    }

    for (const auto &field_init : node->fields) {
        std::string cache_key = struct_type->name + "." + field_init->name.lexeme;
        int field_index = -1;

        auto cache_it = field_index_cache_.find(cache_key);
        if (cache_it != field_index_cache_.end()) {
            field_index = cache_it->second;
        } else {
            for (size_t i = 0; i < struct_type->field_order.size(); ++i) {
                if (struct_type->field_order[i] == field_init->name.lexeme) {
                    field_index = static_cast<int>(i);
                    field_index_cache_[cache_key] = field_index;
                    break;
                }
            }
        }

        if (field_index == -1) {
            continue;
        }

        auto it = struct_type->fields.find(field_init->name.lexeme);
        if (it == struct_type->fields.end()) {
            continue;
        }
        std::string field_ir_type = type_mapper_.map(it->second.get());

        std::vector<std::string> indices = {"i32 0", "i32 " + std::to_string(field_index)};
        std::string field_ptr =
            emitter_.emit_getelementptr_inbounds(struct_ir_type, struct_ptr, indices);

        bool field_is_aggregate =
            (it->second->kind == TypeKind::ARRAY || it->second->kind == TypeKind::STRUCT);

        if (field_is_aggregate) {
            set_target_address(field_ptr);
        }

        field_init->value->accept(this);

        take_target_address();

        std::string field_value = get_expr_result(field_init->value.get());

        if (field_value.empty()) {
            continue;
        }

        if (field_is_aggregate && field_value == field_ptr) {
            continue;
        }

        std::string value_to_store = field_value;

        if (field_is_aggregate) {
            if (loaded_aggregate_results_.find(field_init->value.get()) ==
                loaded_aggregate_results_.end()) {
                value_to_store = emitter_.emit_load(field_ir_type, field_value);
            }
        }

        emitter_.emit_store(field_ir_type, value_to_store, field_ptr);
    }

    store_expr_result(node, struct_ptr);
}

// Generate IR for field access expressions (struct.field).
void IRGenerator::visit(FieldAccessExpr *node) {

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true;
    node->object->accept(this);
    generating_lvalue_ = was_generating_lvalue;

    std::string struct_ptr = get_expr_result(node->object.get());

    if (struct_ptr.empty()) {
        store_expr_result(node, "");
        return;
    }

    std::shared_ptr<Type> actual_type = node->object->type;
    if (actual_type->kind == TypeKind::REFERENCE) {
        auto ref_type = std::dynamic_pointer_cast<ReferenceType>(actual_type);
        if (ref_type) {
            actual_type = ref_type->referenced_type;
        }
    }

    auto struct_type = std::dynamic_pointer_cast<StructType>(actual_type);
    if (!struct_type) {
        store_expr_result(node, "");
        return;
    }

    std::string cache_key = struct_type->name + "." + node->field.lexeme;
    int field_index = -1;

    auto cache_it = field_index_cache_.find(cache_key);
    if (cache_it != field_index_cache_.end()) {
        field_index = cache_it->second;
    } else {
        for (size_t i = 0; i < struct_type->field_order.size(); ++i) {
            if (struct_type->field_order[i] == node->field.lexeme) {
                field_index = static_cast<int>(i);
                field_index_cache_[cache_key] = field_index;
                break;
            }
        }
    }

    if (field_index == -1) {
        store_expr_result(node, "");
        return;
    }

    std::string struct_ir_type = type_mapper_.map(struct_type.get());
    std::string field_ir_type = type_mapper_.map(node->type.get());

    std::vector<std::string> indices = {"i32 0", "i32 " + std::to_string(field_index)};
    std::string field_ptr =
        emitter_.emit_getelementptr_inbounds(struct_ir_type, struct_ptr, indices);

    bool field_is_aggregate =
        (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);

    if (generating_lvalue_ || field_is_aggregate) {
        store_expr_result(node, field_ptr);
    } else {
        std::string field_value = emitter_.emit_load(field_ir_type, field_ptr);
        store_expr_result(node, field_value);
    }
}
