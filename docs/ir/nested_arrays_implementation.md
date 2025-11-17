# 多维数组嵌套实现报告

**实现日期**: 2025-11-13  
**功能模块**: IR 生成器 - 多维数组支持  
**实现阶段**: Phase 2G

## 一、功能概述

本次更新为编译器添加了完整的**多维数组嵌套支持**，使得可以正确处理二维、三维及更高维度的数组类型、初始化和访问操作。

### 支持的功能

1. **多维数组类型** - `[[i32; 3]; 2]`, `[[[i32; 2]; 2]; 2]`
2. **嵌套数组字面量** - `[[1, 2, 3], [4, 5, 6]]`
3. **嵌套初始化器语法** - `[[0; 3]; 2]` (重复初始化)
4. **多维索引访问** - `matrix[0][1]`, `cube[1][1][1]`
5. **元素修改** - `grid[0][1] = 99`
6. **混合嵌套** - 数组中的结构体，如 `[[Point; 2]; 2]`

## 二、技术实现

### 2.1 问题分析

在实现多维数组时，发现了三个核心问题：

#### 问题 1：嵌套数组字面量的 store 类型不匹配

**症状**：

```llvm
%1 = alloca [2 x i32]           ; 内层数组，返回指针
store [2 x i32] %1, ...         ; ❌ 错误：%1是指针，不是值
```

**原因**：当数组元素本身是聚合类型（数组/结构体）时，子表达式返回的是指针，但`store`指令需要的是完整的值。

**解决方案**：在`visit(ArrayLiteralExpr*)`中检查元素类型，如果是聚合类型则先 load：

```cpp
// src/ir/ir_generator.cpp, line ~780
bool elem_is_aggregate = false;
if (auto elem_type = node->elements[i]->type) {
    elem_is_aggregate = (elem_type->kind == TypeKind::ARRAY ||
                         elem_type->kind == TypeKind::STRUCT);
}

if (elem_is_aggregate) {
    elem_value = emitter_.emit_load(elem_ir_type, elem_value);
}
```

#### 问题 2：多维数组索引访问的 load/pointer 问题

**症状**：

```llvm
%11 = load [2 x i32], [2 x i32]* %10    ; matrix[0] 返回值
%13 = getelementptr [2 x i32], [2 x i32]* %12  ; ❌ %12应该是指针，但实际是值
```

**原因**：`IndexExpr`在非左值时默认 load 元素，但对于中间维度的数组访问（如`matrix[0]`后面还有`[1]`），需要返回指针而不是值。

**解决方案**：检查元素类型，如果是聚合类型则不 load：

```cpp
// src/ir/ir_generator.cpp, line ~1196
bool elem_is_aggregate = (array_type->element_type->kind == TypeKind::ARRAY ||
                          array_type->element_type->kind == TypeKind::STRUCT);

if (generating_lvalue_ || elem_is_aggregate) {
    store_expr_result(node, elem_ptr);  // 返回指针
} else {
    std::string elem_value = emitter_.emit_load(elem_ir_type, elem_ptr);
    store_expr_result(node, elem_value);  // 返回值
}
```

**关键决策**：聚合类型的中间访问始终返回指针，只有最终的标量类型才 load。

#### 问题 3：嵌套初始化器的值传递

**症状**：

```rust
let zeros: [[i32; 3]; 2] = [[0; 3]; 2];
// 内层 [0; 3] 生成指针，外层循环需要整个数组值
```

**原因**：与问题 1 类似，`ArrayInitializerExpr`的初始值如果是聚合类型，需要 load 完整值。

**解决方案**：在`visit(ArrayInitializerExpr*)`中添加类型检查：

```cpp
// src/ir/ir_generator.cpp, line ~851
bool value_is_aggregate = false;
if (node->value->type) {
    value_is_aggregate = (node->value->type->kind == TypeKind::ARRAY ||
                         node->value->type->kind == TypeKind::STRUCT);
}

if (value_is_aggregate) {
    init_value = emitter_.emit_load(elem_ir_type, init_value);
}
```

### 2.2 代码修改总结

| 文件                      | 修改内容                          | 行数 |
| ------------------------- | --------------------------------- | ---- |
| `src/ir/ir_generator.cpp` | ArrayLiteralExpr 聚合类型元素处理 | +9   |
| `src/ir/ir_generator.cpp` | IndexExpr 聚合类型判断逻辑        | +5   |
| `src/ir/ir_generator.cpp` | ArrayInitializerExpr 聚合值处理   | +9   |

**总修改量**：约 23 行核心逻辑

### 2.3 生成的 IR 示例

#### 二维数组字面量

**Rust 代码**：

```rust
let matrix: [[i32; 2]; 2] = [[1, 2], [3, 4]];
```

**生成 IR**：

```llvm
%0 = alloca [2 x [2 x i32]]           ; 外层数组
%1 = alloca [2 x i32]                 ; 内层数组1
%2 = getelementptr inbounds [2 x i32], [2 x i32]* %1, i64 0, i64 0
store i32 1, i32* %2
%3 = getelementptr inbounds [2 x i32], [2 x i32]* %1, i64 0, i64 1
store i32 2, i32* %3
%4 = load [2 x i32], [2 x i32]* %1    ; ✓ load整个数组值
%5 = getelementptr inbounds [2 x [2 x i32]], [2 x [2 x i32]]* %0, i64 0, i64 0
store [2 x i32] %4, [2 x i32]* %5     ; ✓ store值而不是指针
```

#### 多维索引访问

**Rust 代码**：

```rust
matrix[0][1]
```

**生成 IR**：

```llvm
%11 = getelementptr inbounds [2 x [2 x i32]], ... , i64 0, i64 0
; ✓ %11 是 [2 x i32]*，不load（因为元素是数组）

%12 = getelementptr inbounds [2 x i32], [2 x i32]* %11, i64 0, i64 1
; ✓ 第二次索引使用%11指针

%13 = load i32, i32* %12
; ✓ 最终标量类型才load
```

## 三、测试验证

### 3.1 测试用例

创建了 `test1/ir/ir_generator/test_nested_arrays.rs`，包含 6 个测试函数：

1. **test_2d_array_basic** - 二维数组字面量和索引
2. **test_2d_array_init_syntax** - 嵌套初始化器 `[[0; 3]; 2]`
3. **test_2d_array_mutation** - 二维数组元素修改
4. **test_3d_array** - 三维数组字面量
5. **test_3d_array_init** - 三维嵌套初始化器
6. **test_array_of_structs_2d** - 结构体数组的字段访问

### 3.2 测试脚本

创建了 `scripts/test_nested_arrays.sh`，执行以下验证：

- ✅ IR 语法正确性（llvm-as 验证）
- ✅ 二维/三维数组类型生成
- ✅ 嵌套索引访问（多次 getelementptr）
- ✅ 嵌套初始化器（内层 load）
- ✅ 元素修改操作
- ✅ 结构体数组字段访问

### 3.3 测试结果

```bash
$ ./scripts/test_nested_arrays.sh

=========================================
   多维数组嵌套功能验证
=========================================

测试1: 二维/三维数组完整测试
  生成IR... ✓ 验证IR语法... ✓ 通过

=== 详细功能验证 ===
检查关键IR特征:
  ✓ 二维数组类型正确生成
  ✓ 三维数组类型正确生成
  ✓ 嵌套索引访问正确实现 (12个getelementptr)
  ✓ 嵌套初始化器正确load内层数组
  ✓ 二维数组元素修改正确
  ✓ 三维数组索引访问 cube[1][1][1] 正确
  ✓ 结构体数组字段访问 points[1][1].x 正确

=========================================
           验证结果
=========================================
✅ 所有多维数组测试通过！
```

**验证状态**: 100% 通过（7/7 功能点）

## 四、设计决策

### 4.1 核心原则

**聚合类型传递规则**：

- 数组和结构体在内存中以指针形式存在（alloca 返回指针）
- 需要值传递时（如 store 到另一个位置），必须先 load 完整值
- 中间访问（如多维索引、字段访问）保持指针形式，避免不必要的 load

### 4.2 类型判断标准

```cpp
bool is_aggregate = (type->kind == TypeKind::ARRAY ||
                     type->kind == TypeKind::STRUCT);
```

这个判断用于决定何时需要特殊处理：

- ArrayLiteralExpr 的元素
- ArrayInitializerExpr 的初始值
- IndexExpr 的返回值
- FieldAccessExpr 的返回值

### 4.3 与现有 Phase 2F 的协同

Phase 2F 已经实现了函数参数的聚合类型处理：

- 函数参数：聚合类型使用指针传递
- 函数返回：聚合类型 load 后返回，调用方 alloca+store

本次 Phase 2G 补充了**表达式层面**的聚合类型处理，两者形成完整的聚合类型支持体系。

## 五、性能考虑

### 5.1 内存布局

多维数组在内存中按行主序（row-major）连续存储：

```
[[1, 2], [3, 4]]  →  [1, 2, 3, 4]  (内存布局)
```

LLVM 的`getelementptr`自动处理多维偏移计算。

### 5.2 优化机会

当前实现为正确性优先，存在优化空间：

1. **临时数组优化** - 内层数组字面量创建临时 alloca，可能可以内联
2. **load/store 优化** - 小数组可以考虑拆分为标量操作
3. **循环展开** - 已在 ArrayInitializerExpr 中实现（<=16 展开）

但这些优化留给 LLVM 后端处理更合适。

## 六、已知限制

1. **类型推导** - 仍需要显式类型标注，如 `let val: i32 = matrix[0][0]`
2. **边界检查** - 运行时不进行数组越界检查
3. **动态大小** - 仅支持编译期常量大小，不支持 VLA
4. **切片** - 不支持数组切片语法 `arr[..]`

## 七、后续建议

### 7.1 短期优化

- 添加编译期越界检查（常量索引）
- 支持数组长度查询（`.len()`方法需要 impl 块支持）
- 改进错误消息（指明是哪一维度出错）

### 7.2 长期扩展

- 支持多维数组的迭代器
- 支持矩阵操作（如转置、矩阵乘法等内置函数）
- SIMD 优化（向量化数组操作）

## 八、文档更新

已创建/更新以下文档：

- `docs/ir/nested_arrays_implementation.md` - 本文档
- `docs/ir/impl_implementation_plan.md` - impl 块实现计划
- `scripts/test_nested_arrays.sh` - 专项测试脚本

## 九、总结

Phase 2G 成功实现了多维数组的完整支持，核心修改仅 23 行，通过以下关键决策解决了聚合类型传递问题：

1. **识别聚合类型** - 统一判断标准 `TypeKind::ARRAY || STRUCT`
2. **load 时机** - 需要值时 load，需要进一步访问时保持指针
3. **递归处理** - 利用现有机制自然支持任意维度嵌套

测试验证显示所有功能 100%通过，IR 生成正确且符合 LLVM 规范。这为后续的 impl 块、方法和更高级特性奠定了坚实基础。

---

**实现者**: GitHub Copilot  
**审核状态**: 待审核  
**测试状态**: ✅ 全部通过
