# Phase 2H 优化报告：Self 参数与引用表达式

## 日期

2024 年 11 月 13 日

## 概述

本次优化主要解决了两个关键问题：

1. **Self 参数双重指针问题**：优化 self 参数处理，避免不必要的 alloca 层
2. **引用表达式支持**：实现 ReferenceExpr 的 IR 生成，支持显式引用参数

## 1. Self 参数优化

### 问题描述

在之前的实现中，对于带有 `&self` 或 `&mut self` 参数的方法，IR 生成会创建双重指针：

```llvm
; 问题代码
define i32 @Point_get_x(%Point* %self) {
  %0 = alloca %Point*         ; ❌ 创建了额外的指针alloca
  store %Point* %self, %Point** %0
  %1 = load %Point*, %Point** %0
  %2 = getelementptr inbounds %Point, %Point* %1, i32 0, i32 0
  ...
}
```

### 根本原因

引用类型参数（`&self`）在函数签名中已经是指针类型 `%Point*`，但参数处理逻辑仍然为其创建了 `alloca %Point**`，导致：

- 不必要的内存分配
- 额外的 load/store 操作
- 类型不匹配的潜在风险

### 解决方案

#### 修改 1：函数参数处理 (`visit_function_decl`)

在 `src/ir/ir_generator.cpp` 的函数声明处理中，添加引用类型检查：

```cpp
// 第101行附近
bool is_reference = (param->type->resolved_type->kind == TypeKind::REFERENCE);

if ((is_aggregate && param_is_aggregate[i]) || is_reference) {
    // 引用类型和聚合类型：不创建alloca，直接注册
    value_manager_.define_variable(param_name, param_ir_name,
                                   param_type_str, is_mutable);
} else {
    // 基础类型：创建alloca并存储
    std::string alloca_name = emitter_.emit_alloca(param_type_str);
    emitter_.emit_store(param_type_str, param_ir_name, alloca_name);
    value_manager_.define_variable(param_name, alloca_name,
                                   param_type_str, is_mutable);
}
```

#### 修改 2：变量表达式处理 (`visit(VariableExpr*)`)

在变量访问时，引用类型应直接返回指针，不进行 load：

```cpp
// 第522行附近
bool is_reference = (node->type->kind == TypeKind::REFERENCE);

if (is_aggregate || is_reference) {
    // 聚合类型和引用类型：返回指针
    store_expr_result(node, var_info->alloca_name);
} else {
    // 基础类型：加载值
    std::string value = emitter_.emit_load(var_type_str, var_info->alloca_name);
    store_expr_result(node, value);
}
```

#### 修改 3：字段访问处理 (`visit(FieldAccessExpr*)`)

处理引用类型对象的字段访问：

```cpp
// 第1374行附近
std::shared_ptr<Type> actual_type = node->object->type;
if (actual_type->kind == TypeKind::REFERENCE) {
    // 解引用获取实际的结构体类型
    auto ref_type = std::dynamic_pointer_cast<ReferenceType>(actual_type);
    if (ref_type) {
        actual_type = ref_type->referenced_type;
    }
}
```

### 优化效果

#### 优化前

```llvm
define i32 @Point_get_x(%Point* %self) {
  %0 = alloca %Point*              ; 额外的alloca
  store %Point* %self, %Point** %0  ; 额外的store
  %1 = load %Point*, %Point** %0    ; 额外的load
  %2 = getelementptr inbounds %Point, %Point* %1, i32 0, i32 0
  %3 = load i32, i32* %2
  ret i32 %3
}
```

#### 优化后

```llvm
define i32 @Point_get_x(%Point* %self) {
  %0 = getelementptr inbounds %Point, %Point* %self, i32 0, i32 0  ; 直接使用
  %1 = load i32, i32* %0
  ret i32 %1
}
```

**性能提升：**

- 减少 3 条指令（alloca、store、load）
- 减少栈内存使用（8 字节指针）
- 更清晰的 IR，易于 LLVM 优化

## 2. ReferenceExpr 实现

### 问题描述

在方法调用时，显式传递的引用参数（如 `obj.method(&other)`）会丢失，导致：

```rust
p1.add(&p2)  // ❌ 生成：call @Point_add(%Point* %p1)
             // ✅ 期望：call @Point_add(%Point* %p1, %Point* %p2)
```

### 根本原因

1. `visit(ReferenceExpr*)` 是空实现，未生成任何 IR
2. 语义分析未为 `ReferenceExpr` 设置类型信息
3. `visit(CallExpr*)` 跳过了无类型的参数

### 解决方案

#### 修改 1：实现 ReferenceExpr 的 IR 生成

```cpp
void IRGenerator::visit(ReferenceExpr *node) {
    // 计算内部表达式
    node->expression->accept(this);
    std::string value = get_expr_result(node->expression.get());

    if (value.empty()) {
        store_expr_result(node, "");
        return;
    }

    // Workaround：如果语义分析未设置类型，手动推导
    if (!node->type && node->expression->type) {
        node->type = std::make_shared<ReferenceType>(
            node->expression->type, false);
    }

    // 对于聚合类型，表达式结果已经是指针，直接返回
    if (node->expression->type) {
        bool is_aggregate = (node->expression->type->kind == TypeKind::ARRAY ||
                             node->expression->type->kind == TypeKind::STRUCT);

        if (is_aggregate) {
            store_expr_result(node, value);  // 直接传递指针
        } else {
            // 基础类型也返回值（通常也是alloca指针）
            store_expr_result(node, value);
        }
    } else {
        store_expr_result(node, value);
    }
}
```

#### 修改 2：CallExpr 参数处理支持引用类型

```cpp
for (const auto &arg : node->arguments) {
    arg->accept(this);
    std::string arg_value = get_expr_result(arg.get());

    if (arg_value.empty() || !arg->type) {
        continue;  // 跳过无效参数
    }

    std::string arg_type_str = type_mapper_.map(arg->type.get());

    bool is_aggregate = (arg->type->kind == TypeKind::ARRAY ||
                         arg->type->kind == TypeKind::STRUCT);
    bool is_reference = (arg->type->kind == TypeKind::REFERENCE);

    if (is_aggregate || is_reference) {
        if (is_reference) {
            // 引用类型：解引用获取实际类型
            auto ref_type = std::dynamic_pointer_cast<ReferenceType>(arg->type);
            if (ref_type && ref_type->referenced_type) {
                std::string actual_type_str =
                    type_mapper_.map(ref_type->referenced_type.get());
                args.push_back({actual_type_str + "*", arg_value});
            }
        } else {
            args.push_back({arg_type_str + "*", arg_value});
        }
    } else {
        args.push_back({arg_type_str, arg_value});
    }
}
```

### 实现效果

#### 测试代码

```rust
struct Point { x: i32, y: i32 }

impl Point {
    fn add(&self, other: &Point) -> i32 {
        self.x + other.x
    }
}

fn test() -> i32 {
    let p1: Point = Point { x: 1, y: 2 };
    let p2: Point = Point { x: 3, y: 4 };
    p1.add(&p2)
}
```

#### 生成的 IR

```llvm
define i32 @Point_add(%Point* %self, %Point* %other) {
entry:
  %0 = getelementptr inbounds %Point, %Point* %self, i32 0, i32 0
  %1 = load i32, i32* %0
  %2 = getelementptr inbounds %Point, %Point* %other, i32 0, i32 0
  %3 = load i32, i32* %2
  %4 = add i32 %1, %3
  ret i32 %4
}

define i32 @test() {
entry:
  %0 = alloca %Point
  ; ... 初始化 p1 ...
  %3 = alloca %Point
  ; ... 初始化 p2 ...
  %6 = call i32 @Point_add(%Point* %0, %Point* %3)  ; ✅ 两个参数都传递了
  ret i32 %6
}
```

## 3. 测试覆盖

### 新增测试文件

1. **test_edge_cases.rs** - 边缘案例测试

   - 方法链式调用
   - 嵌套方法调用
   - 数组元素作为接收者
   - 多个引用参数

2. **test_complex_scenarios.rs** - 复杂场景测试
   - 结构体数组的方法调用
   - 二维数组配合结构体
   - 结构体组合
   - 方法返回结构体并链式调用

### 测试结果

```
总计: 18个测试
通过: 18个 (100%)
失败: 0个
```

## 4. 性能影响

### 指令数量优化

- **Self 参数优化**：每个方法调用减少 3 条指令
- **引用传递**：正确性修复，无额外开销

### 内存使用

- 每个带 self 参数的方法减少 8 字节栈使用（64 位系统）
- 对于频繁调用的方法，累积效果显著

## 5. 局限性与未来改进

### 当前局限

1. **语义分析缺失**：ReferenceExpr 类型推导在 IR 生成阶段 workaround
2. **编译器警告**：已清理 `has_self` unused 变量警告

### 建议改进

1. 在语义分析阶段为 ReferenceExpr 正确设置类型
2. 支持更复杂的引用场景（如 `&&T`、`&mut &T`）
3. 实现引用的解引用操作符 `*`

## 6. 总结

本次优化成功解决了：

- ✅ Self 参数双重指针问题
- ✅ 引用表达式 IR 生成
- ✅ 方法调用参数传递
- ✅ 18 个测试全部通过

**代码质量提升：**

- 生成的 IR 更简洁高效
- 消除了编译器警告
- 增加了测试覆盖率（+6 个测试）

**下一步方向：**

- 修复语义分析阶段的 ReferenceExpr 类型推导
- 支持 trait 和泛型
- 实现闭包和高阶函数
