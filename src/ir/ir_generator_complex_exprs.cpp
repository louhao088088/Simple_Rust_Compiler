/**
 * @file ir_generator_complex_exprs.cpp
 * @brief IR生成器 - 复杂表达式处理模块
 *
 * 本文件包含复杂Expression节点的IR生成实现：
 * - ArrayLiteralExpr: 数组字面量 [1, 2, 3]
 * - ArrayInitializerExpr: 数组初始化器 [value; size]
 * - IndexExpr: 数组索引访问 arr[i]
 * - StructInitializerExpr: 结构体初始化 Point { x: 1, y: 2 }
 * - FieldAccessExpr: 结构体字段访问 obj.field
 *
 * 这些表达式涉及：
 * - 聚合类型的内存分配和初始化
 * - getelementptr指令的使用
 * - 左值/右值的区分
 * - 嵌套数组和结构体的处理
 */

#include "ir_generator.h"

// ========== 复杂表达式 Visitors ==========

void IRGenerator::visit(ArrayLiteralExpr *node) {
    // 数组字面量: [1, 2, 3, 4, 5]
    //
    // 生成步骤:
    // 1. 在栈上分配数组空间 (alloca [N x type])
    // 2. 逐个初始化元素 (使用 getelementptr + store)
    // 3. 返回数组指针

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    // 获取数组类型信息
    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->type);
    if (!array_type) {
        store_expr_result(node, "");
        return;
    }

    // 获取元素类型和数组大小
    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    size_t array_size = node->elements.size();
    std::string array_ir_type = "[" + std::to_string(array_size) + " x " + elem_ir_type + "]";

    // 1. 分配数组空间
    // 优化：如果存在目标地址（原地初始化），直接使用
    std::string array_ptr = take_target_address();
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    // 2. 初始化每个元素
    for (size_t i = 0; i < node->elements.size(); ++i) {
        // 计算元素的值
        node->elements[i]->accept(this);
        std::string elem_value = get_expr_result(node->elements[i].get());

        if (elem_value.empty()) {
            continue; // 跳过未能生成的元素
        }

        // 如果元素类型是聚合类型(数组/结构体)，需要先load整个值
        // 因为子表达式返回的是指针，但store需要的是值
        bool elem_is_aggregate = false;
        if (auto elem_type = node->elements[i]->type) {
            elem_is_aggregate =
                (elem_type->kind == TypeKind::ARRAY || elem_type->kind == TypeKind::STRUCT);
        }

        if (elem_is_aggregate) {
            // Load整个聚合值
            elem_value = emitter_.emit_load(elem_ir_type, elem_value);
        }

        // 计算元素地址: getelementptr [N x T], ptr %arr, i64 0, i64 i
        std::vector<std::string> indices = {"i64 0", "i64 " + std::to_string(i)};
        std::string elem_ptr =
            emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

        // 存储元素值
        emitter_.emit_store(elem_ir_type, elem_value, elem_ptr);
    }

    // 3. 返回数组指针
    store_expr_result(node, array_ptr);
}

void IRGenerator::visit(ArrayInitializerExpr *node) {
    // 数组初始化表达式: [value; size]
    // 例如: let arr = [0; 100];  创建100个0
    //
    // 生成策略:
    // 1. 在栈上分配数组空间 (alloca [N x type])
    // 2. 使用循环初始化所有元素为相同值
    //    - 对于小数组(<=16)，展开为多个store
    //    - 对于大数组(>16)，使用循环

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    // 获取数组类型信息
    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->type);
    if (!array_type) {
        store_expr_result(node, "");
        return;
    }

    // 优化：尽早获取目标地址，防止子表达式错误使用
    std::string target_ptr = take_target_address();

    // 获取数组大小
    size_t array_size = array_type->size;

    // 获取元素类型
    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    std::string array_ir_type = "[" + std::to_string(array_size) + " x " + elem_ir_type + "]";

    // 检查初始化值是否是聚合类型(数组/结构体)
    bool value_is_aggregate = false;
    if (node->value->type) {
        value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                              node->value->type->kind == TypeKind::STRUCT);
    }

    // 计算初始化值
    // 如果值是聚合类型，为它分配临时空间作为目标地址，避免误用外层 SRET 指针
    if (value_is_aggregate) {
        std::string temp_ptr = emitter_.emit_alloca(elem_ir_type);
        set_target_address(temp_ptr);
    }

    node->value->accept(this);

    // 清除未使用的target
    take_target_address();

    std::string init_value = get_expr_result(node->value.get());

    if (init_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 如果初始化值是聚合类型，需要load整个值
    // 因为子表达式返回的是指针，但store需要的是值
    if (value_is_aggregate) {
        init_value = emitter_.emit_load(elem_ir_type, init_value);
    }

    // 1. 分配数组空间
    // 优化：如果存在目标地址（原地初始化），直接使用
    std::string array_ptr = target_ptr;
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    // 2. 初始化元素
    const size_t UNROLL_THRESHOLD = 16; // 小于等于16个元素时展开
    const size_t MEMSET_THRESHOLD = 64; // 大于64个元素且是零初始化时使用memset

    // 检查是否是零初始化（递归检查整数和结构体）
    bool is_zero_init = is_zero_initializer(node->value.get());

    // 计算元素大小(用于memset优化)
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
        // 结构体类型,使用AST类型计算大小
        elem_size = get_type_size(array_type->element_type.get());
    }

    // 优化：对于大数组的零初始化且能计算大小，使用memset
    if (is_zero_init && array_size > MEMSET_THRESHOLD && elem_size > 0) {
        size_t total_bytes = array_size * elem_size;

        // 调用memset: memset(ptr, 0, size)
        emitter_.emit_memset(array_ptr, 0, total_bytes, array_ir_type + "*");
    } else if (array_size <= UNROLL_THRESHOLD) {
        // 小数组：展开为多个store指令
        for (size_t i = 0; i < array_size; ++i) {
            // 计算元素地址: getelementptr [N x T], ptr %arr, i64 0, i64 i
            std::vector<std::string> indices = {"i64 0", "i64 " + std::to_string(i)};
            std::string elem_ptr =
                emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

            // 存储元素值
            emitter_.emit_store(elem_ir_type, init_value, elem_ptr);
        }
    } else {
        // 大数组：使用循环初始化
        // 生成循环结构:
        //   %i = alloca i64
        //   store i64 0, i64* %i
        //   br label %init_loop_cond
        // init_loop_cond:
        //   %i_val = load i64, i64* %i
        //   %cmp = icmp slt i64 %i_val, N
        //   br i1 %cmp, label %init_loop_body, label %init_loop_end
        // init_loop_body:
        //   %elem_ptr = getelementptr [N x T], ptr %arr, i64 0, i64 %i_val
        //   store T init_value, T* %elem_ptr
        //   %next_i = add i64 %i_val, 1
        //   store i64 %next_i, i64* %i
        //   br label %init_loop_cond
        // init_loop_end:

        std::string loop_cond_label = emitter_.new_label();
        std::string loop_body_label = emitter_.new_label();
        std::string loop_end_label = emitter_.new_label();

        // 循环变量i
        std::string i_ptr = emitter_.emit_alloca("i64");
        emitter_.emit_store("i64", "0", i_ptr);
        emitter_.emit_br(loop_cond_label);

        // 条件块
        begin_block(loop_cond_label);
        std::string i_val = emitter_.emit_load("i64", i_ptr);
        std::string cmp = emitter_.emit_icmp("slt", "i64", i_val, std::to_string(array_size));
        emitter_.emit_cond_br(cmp, loop_body_label, loop_end_label);

        // 循环体
        begin_block(loop_body_label);
        std::vector<std::string> indices = {"i64 0", "i64 " + i_val};
        std::string elem_ptr =
            emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);
        emitter_.emit_store(elem_ir_type, init_value, elem_ptr);

        std::string next_i = emitter_.emit_binary_op("add", "i64", i_val, "1");
        emitter_.emit_store("i64", next_i, i_ptr);
        emitter_.emit_br(loop_cond_label);

        // 循环结束
        begin_block(loop_end_label);
    }

    // 3. 返回数组指针
    store_expr_result(node, array_ptr);
}

void IRGenerator::visit(IndexExpr *node) {
    // 数组索引: arr[i]
    //
    // 生成步骤:
    // 1. 计算数组对象表达式，得到数组指针
    // 2. 计算索引表达式，得到索引值
    // 3. 使用 getelementptr 计算元素地址
    // 4. 如果是右值，load 加载值；如果是左值，返回指针

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    // 1. 计算数组对象（得到数组指针）
    // 注意：即使IndexExpr本身是右值，对象也必须产生指针（用于getelementptr）
    // 因此，我们临时设置generating_lvalue_=true来获取对象的地址
    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true; // 数组对象必须返回指针
    node->object->accept(this);
    generating_lvalue_ = was_generating_lvalue; // 恢复原来的标志

    std::string array_ptr = get_expr_result(node->object.get());

    if (array_ptr.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 2. 计算索引值（索引始终是右值，需要加载其值）
    generating_lvalue_ = false; // 索引表达式必须产生右值
    node->index->accept(this);
    generating_lvalue_ = was_generating_lvalue; // 恢复原来的标志

    std::string index_value = get_expr_result(node->index.get());

    if (index_value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 索引可能是i32（usize on 32-bit），需要扩展到i64用于getelementptr
    std::string index_type_str = type_mapper_.map(node->index->type.get());
    if (index_type_str == "i32") {
        // 扩展i32到i64（使用sext for isize, zext for usize）
        bool is_signed = (node->index->type->kind == TypeKind::ISIZE ||
                          node->index->type->kind == TypeKind::I32);
        if (is_signed) {
            index_value = emitter_.emit_sext("i32", index_value, "i64");
        } else {
            index_value = emitter_.emit_zext("i32", index_value, "i64");
        }
    }

    // 获取数组类型信息
    // 如果对象类型是引用类型，需要解引用到实际的数组类型
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

    // 获取元素类型和数组大小
    std::string elem_ir_type = type_mapper_.map(array_type->element_type.get());
    std::string array_ir_type = "[" + std::to_string(array_type->size) + " x " + elem_ir_type + "]";

    // 3. 计算元素地址: getelementptr [N x T], ptr %arr, i64 0, i64 %index
    // 第一个索引0是解引用数组指针，第二个索引是数组内的偏移
    std::vector<std::string> indices = {"i64 0", "i64 " + index_value};
    std::string elem_ptr = emitter_.emit_getelementptr_inbounds(array_ir_type, array_ptr, indices);

    // 4. 根据上下文决定是加载值还是返回指针
    // 只有在生成左值时（用于赋值），才返回指针；否则加载值
    // 但是，如果元素类型是聚合类型(数组/结构体)，则返回指针（用于进一步访问）
    bool elem_is_aggregate = (array_type->element_type->kind == TypeKind::ARRAY ||
                              array_type->element_type->kind == TypeKind::STRUCT);

    if (generating_lvalue_ || elem_is_aggregate) {
        // 左值或聚合类型：返回元素指针
        store_expr_result(node, elem_ptr);
    } else {
        // 右值（非聚合）：加载元素值
        std::string elem_value = emitter_.emit_load(elem_ir_type, elem_ptr);
        store_expr_result(node, elem_value);
    }
}

void IRGenerator::visit(StructInitializerExpr *node) {
    // 结构体初始化: Point { x: 10, y: 20 }
    //
    // 生成步骤:
    // 1. 分配结构体空间
    // 2. 逐个字段初始化（使用 getelementptr + store）
    // 3. 返回结构体指针

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    // 获取结构体类型
    auto struct_type = std::dynamic_pointer_cast<StructType>(node->type);
    if (!struct_type) {
        store_expr_result(node, "");
        return;
    }

    // 获取结构体IR类型
    std::string struct_ir_type = type_mapper_.map(struct_type.get());

    // 1. 分配结构体空间
    // 如果存在 __sret_self 变量，说明当前在 sret 优化的构造函数中
    // 直接使用 self 而不是 alloca
    std::string struct_ptr;

    // 优化：优先使用目标地址（原地初始化）
    std::string target_ptr = take_target_address();

    auto sret_self = value_manager_.lookup_variable("__sret_self");
    if (!target_ptr.empty()) {
        struct_ptr = target_ptr;
    } else if (sret_self) {
        // 使用 sret self 指针
        struct_ptr = sret_self->alloca_name;
    } else {
        // 正常 alloca
        struct_ptr = emitter_.emit_alloca(struct_ir_type);
    }

    // 2. 初始化每个字段
    for (const auto &field_init : node->fields) {
        // 查找字段在field_order中的索引（使用缓存）
        std::string cache_key = struct_type->name + "." + field_init->name.lexeme;
        int field_index = -1;

        auto cache_it = field_index_cache_.find(cache_key);
        if (cache_it != field_index_cache_.end()) {
            field_index = cache_it->second;
        } else {
            // 首次查找，线性搜索并缓存
            for (size_t i = 0; i < struct_type->field_order.size(); ++i) {
                if (struct_type->field_order[i] == field_init->name.lexeme) {
                    field_index = static_cast<int>(i);
                    field_index_cache_[cache_key] = field_index;
                    break;
                }
            }
        }

        if (field_index == -1) {
            continue; // 字段未找到
        }

        // 获取字段类型
        auto it = struct_type->fields.find(field_init->name.lexeme);
        if (it == struct_type->fields.end()) {
            continue;
        }
        std::string field_ir_type = type_mapper_.map(it->second.get());

        // 计算字段地址: getelementptr %StructType, ptr %struct, i32 0, i32 field_index
        std::vector<std::string> indices = {"i32 0", "i32 " + std::to_string(field_index)};
        std::string field_ptr =
            emitter_.emit_getelementptr_inbounds(struct_ir_type, struct_ptr, indices);

        // 优化：对于聚合类型字段，尝试原地初始化
        bool field_is_aggregate =
            (it->second->kind == TypeKind::ARRAY || it->second->kind == TypeKind::STRUCT);

        if (field_is_aggregate) {
            set_target_address(field_ptr);
        }

        // 计算字段值
        field_init->value->accept(this);

        // 清除未使用的target（如果子表达式没有使用它）
        take_target_address();

        std::string field_value = get_expr_result(field_init->value.get());

        if (field_value.empty()) {
            continue; // 跳过未能生成的字段
        }

        // 如果发生了原地初始化，field_value 应该是 field_ptr
        // 此时不需要 store
        if (field_is_aggregate && field_value == field_ptr) {
            continue;
        }

        std::string value_to_store = field_value;

        if (field_is_aggregate) {
            // 检查field_value是否已经是值
            if (loaded_aggregate_results_.find(field_init->value.get()) ==
                loaded_aggregate_results_.end()) {
                // 还是指针，需要load
                value_to_store = emitter_.emit_load(field_ir_type, field_value);
            }
        }

        // 存储字段值
        emitter_.emit_store(field_ir_type, value_to_store, field_ptr);
    }

    // 3. 返回结构体指针
    store_expr_result(node, struct_ptr);
}

void IRGenerator::visit(FieldAccessExpr *node) {
    // 结构体字段访问: obj.field
    //
    // 生成步骤:
    // 1. 计算对象表达式，得到结构体指针
    // 2. 获取字段在结构体中的索引
    // 3. 使用 getelementptr 计算字段地址
    // 4. 如果是右值，load 加载值；如果是左值，返回指针

    if (!node->type) {
        store_expr_result(node, "");
        return;
    }

    // 1. 计算对象表达式（得到结构体指针）
    // 注意：即使FieldAccessExpr本身是右值，对象也必须产生指针（用于getelementptr）
    // 因此，我们临时设置generating_lvalue_=true来获取对象的地址
    bool was_generating_lvalue = generating_lvalue_;
    generating_lvalue_ = true; // 结构体对象必须返回指针
    node->object->accept(this);
    generating_lvalue_ = was_generating_lvalue; // 恢复原来的标志

    std::string struct_ptr = get_expr_result(node->object.get());

    if (struct_ptr.empty()) {
        store_expr_result(node, "");
        return;
    }

    // 获取结构体类型
    // 如果对象类型是引用类型，需要解引用到实际的结构体类型
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

    // 2. 从 field_order 中获取字段索引（使用缓存）
    std::string cache_key = struct_type->name + "." + node->field.lexeme;
    int field_index = -1;

    auto cache_it = field_index_cache_.find(cache_key);
    if (cache_it != field_index_cache_.end()) {
        field_index = cache_it->second;
    } else {
        // 首次查找，线性搜索并缓存
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

    // 获取结构体IR类型和字段IR类型
    std::string struct_ir_type = type_mapper_.map(struct_type.get());
    std::string field_ir_type = type_mapper_.map(node->type.get());

    // 3. 计算字段地址: getelementptr %StructType, ptr %struct, i32 0, i32 field_index
    std::vector<std::string> indices = {
        "i32 0",                             // 解引用结构体指针
        "i32 " + std::to_string(field_index) // 字段索引
    };
    std::string field_ptr =
        emitter_.emit_getelementptr_inbounds(struct_ir_type, struct_ptr, indices);

    // 4. 根据上下文决定是加载值还是返回指针
    // 对于聚合类型字段，始终返回指针（与IndexExpr的聚合元素处理一致）
    // 只有非聚合类型的字段才在右值模式下load
    bool field_is_aggregate =
        (node->type->kind == TypeKind::ARRAY || node->type->kind == TypeKind::STRUCT);

    if (generating_lvalue_ || field_is_aggregate) {
        // 左值或聚合类型字段：返回字段指针
        store_expr_result(node, field_ptr);
    } else {
        // 右值（非聚合）：加载字段值
        std::string field_value = emitter_.emit_load(field_ir_type, field_ptr);
        store_expr_result(node, field_value);
    }
}
