# 复杂表达式生成

本模块实现数组和结构体相关的复杂表达式。

## 文件位置

`src/ir/ir_generator_complex_exprs.cpp`

## 数组表达式

### 1. 数组字面量 (`ArrayLiteralExpr`)

```rust
let arr = [1, 2, 3, 4, 5];
```

**生成步骤**:

1. 分配数组空间: `alloca [5 x i32]`
2. 逐个初始化元素: `store i32 1, i32* %elem0`
3. 返回数组指针

**实现**:

```cpp
void IRGenerator::visit(ArrayLiteralExpr *node) {
    std::string array_ptr = take_target_address();
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    for (size_t i = 0; i < node->elements.size(); ++i) {
        node->elements[i]->accept(this);
        std::string elem_value = get_expr_result(node->elements[i].get());

        std::string elem_ptr = emitter_.emit_getelementptr_inbounds(
            array_ir_type, array_ptr, {"i64 0", "i64 " + std::to_string(i)}
        );
        emitter_.emit_store(elem_ir_type, elem_value, elem_ptr);
    }

    store_expr_result(node, array_ptr);
}
```

### 2. 数组初始化器 (`ArrayInitializerExpr`)

```rust
let arr = [0; 1000];  // 1000 个 0
```

**优化策略**:

| 数组大小       | 策略       | 原因               |
| -------------- | ---------- | ------------------ |
| ≤ 16           | 展开 store | 指令少，cache 友好 |
| > 16, ≤ 64     | 循环       | 平衡代码大小和性能 |
| > 64, 零初始化 | memset     | 最快               |
| > 64, 非零     | 循环       | memset 只支持零    |

**实现**:

```cpp
void IRGenerator::visit(ArrayInitializerExpr *node) {
    // 优化: 获取目标地址
    std::string target_ptr = take_target_address();

    node->value->accept(this);
    std::string init_value = get_expr_result(node->value.get());

    std::string array_ptr = target_ptr;
    if (array_ptr.empty()) {
        array_ptr = emitter_.emit_alloca(array_ir_type);
    }

    // 检查是否零初始化
    bool is_zero_init = is_zero_initializer(node->value.get());

    if (is_zero_init && array_size > 64) {
        // 使用 memset
        emitter_.emit_memset(array_ptr, 0, total_bytes, ptr_type);
    } else if (array_size <= 16) {
        // 展开循环
        for (size_t i = 0; i < array_size; ++i) {
            std::string elem_ptr = emit_getelementptr(...);
            emitter_.emit_store(elem_ir_type, init_value, elem_ptr);
        }
    } else {
        // 使用循环
        generate_init_loop(array_ptr, init_value, array_size);
    }
}
```

**Memset 调用**:

```llvm
%0 = bitcast [1000 x i32]* %arr to i8*
call void @llvm.memset.p0.i8(i8* %0, i8 0, i64 4000, i1 false)
```

### 3. 数组索引 (`IndexExpr`)

```rust
let x = arr[i];
arr[j] = 10;
```

**左值 vs 右值**:

**右值**（读取）:

```llvm
%ptr = getelementptr [100 x i32], [100 x i32]* %arr, i64 0, i64 %i
%value = load i32, i32* %ptr
```

**左值**（赋值目标）:

```llvm
%ptr = getelementptr [100 x i32], [100 x i32]* %arr, i64 0, i64 %i
; 返回 %ptr，不 load
```

**实现**:

```cpp
void IRGenerator::visit(IndexExpr *node) {
    node->object->accept(this);
    std::string array_ptr = get_expr_result(node->object.get());

    node->index->accept(this);
    std::string index_value = get_expr_result(node->index.get());

    // 计算元素地址
    std::string elem_ptr = emitter_.emit_getelementptr_inbounds(
        array_type_str, array_ptr, {"i64 0", index_value}
    );

    if (generating_lvalue_) {
        store_expr_result(node, elem_ptr);  // 返回地址
    } else {
        std::string value = emitter_.emit_load(elem_type_str, elem_ptr);
        store_expr_result(node, value);     // 返回值
    }
}
```

## 结构体表达式

### 1. 结构体初始化器 (`StructInitializerExpr`)

```rust
let p = Point { x: 10, y: 20 };
```

**生成步骤**:

1. 分配结构体空间: `alloca %Point`
2. 逐个字段初始化: `store i32 10, i32* %x_ptr`
3. 返回结构体指针

**实现**:

```cpp
void IRGenerator::visit(StructInitializerExpr *node) {
    std::string target_ptr = take_target_address();

    std::string struct_ptr = target_ptr;
    if (struct_ptr.empty()) {
        struct_ptr = emitter_.emit_alloca(struct_type_str);
    }

    for (const auto &[field_name, field_expr] : node->fields) {
        // 获取字段索引
        int field_index = get_field_index(struct_type, field_name);

        // 清除目标地址，避免嵌套结构体误用
        take_target_address();

        field_expr->accept(this);
        std::string field_value = get_expr_result(field_expr.get());

        // 计算字段地址
        std::string field_ptr = emitter_.emit_getelementptr_inbounds(
            struct_type_str, struct_ptr,
            {"i32 0", "i32 " + std::to_string(field_index)}
        );

        // 存储值
        if (is_aggregate_field) {
            size_t size = get_type_size(field_type);
            emitter_.emit_memcpy(field_ptr, field_value, size, ptr_type);
        } else {
            emitter_.emit_store(field_type_str, field_value, field_ptr);
        }
    }

    store_expr_result(node, struct_ptr);
}
```

**关键修复**: 清除 target_address

**问题**: comprehensive19/20 中数组初始化器包含结构体

```rust
let nodes = [Node { data: 0, next: -1 }; 100];
```

**错误行为**: 结构体初始化器误用了数组的 target_address，导致类型冲突

**修复**:

```cpp
// 在处理每个字段前
take_target_address();  // ✅ 清除父级的 target_address
```

### 2. 字段访问 (`FieldAccessExpr`)

```rust
let x = p.x;
p.y = 30;
```

**实现**:

```cpp
void IRGenerator::visit(FieldAccessExpr *node) {
    node->object->accept(this);
    std::string struct_ptr = get_expr_result(node->object.get());

    int field_index = get_field_index(struct_type, field_name);

    std::string field_ptr = emitter_.emit_getelementptr_inbounds(
        struct_type_str, struct_ptr,
        {"i32 0", "i32 " + std::to_string(field_index)}
    );

    if (generating_lvalue_) {
        store_expr_result(node, field_ptr);
    } else {
        std::string value = emitter_.emit_load(field_type_str, field_ptr);
        store_expr_result(node, value);
    }
}
```

**方法调用**: 自动添加 `self` 参数

```rust
p.method(arg)  // 转换为 Point_method(&p, arg)
```

## 嵌套结构

### 嵌套数组

```rust
let matrix = [[0; 10]; 10];  // 10x10 矩阵
```

**类型**: `[[i32; 10]; 10]` → `[10 x [10 x i32]]`

**索引**: `matrix[i][j]`

```llvm
%row_ptr = getelementptr [10 x [10 x i32]], ..., i64 0, i64 %i
%elem_ptr = getelementptr [10 x i32], [10 x i32]* %row_ptr, i64 0, i64 %j
```

### 结构体数组

```rust
let points = [Point { x: 0, y: 0 }; 100];
```

**初始化策略**:

1. 创建结构体模板: `alloca %Point`
2. 初始化模板
3. 循环拷贝模板到数组每个元素

**优化**: 如果结构体是零初始化，使用 memset

## 优化技术

### 1. 目标地址传递

**避免临时变量**:

```rust
let arr = [expensive_func(); 100];
```

不优化:

```llvm
%temp = call i32 @expensive_func()  ; 100次
%elem = alloca i32
store i32 %temp, i32* %elem
; ... 拷贝到数组 ...
```

优化:

```llvm
; 直接在数组元素位置调用
; 无需临时变量
```

### 2. Memcpy/Memset Intrinsics

**性能提升**: 批量内存操作比循环快得多

**测试**: comprehensive14/15 - 大数组操作性能显著提升

### 3. 循环展开

小数组（≤16）展开成直接 store，减少分支开销。

## 测试覆盖

- ✅ 数组字面量
- ✅ 数组初始化器（各种大小）
- ✅ 多维数组
- ✅ 结构体初始化器
- ✅ 嵌套结构体
- ✅ 结构体数组
- ✅ 数组索引
- ✅ 字段访问
- ✅ 方法调用

---

**参见**:

- [辅助工具](./08_helpers.md) - 类型大小计算
- [IR 发射器](./09_ir_emitter.md) - memcpy/memset
- [类型映射](./10_type_mapper.md) - 结构体类型
