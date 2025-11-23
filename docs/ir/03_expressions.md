# 表达式生成

本模块实现基础表达式的 IR 生成，包括字面量、变量、运算符、函数调用等。

## 文件位置

`src/ir/ir_generator_expressions.cpp`

## 表达式生成原则

### 1. 结果存储

所有表达式生成结果后：

```cpp
store_expr_result(node, result_register);
```

父节点通过以下方式获取：

```cpp
std::string result = get_expr_result(child_node);
```

### 2. 类型处理

表达式类型从 AST 节点的 `type` 字段获取（由语义分析器填充）：

```cpp
std::string type_str = type_mapper_.map(node->type.get());
```

### 3. 左值 vs 右值

**左值**（地址）: 用于赋值目标

- IndexExpr: `getelementptr` 返回元素指针
- FieldAccessExpr: `getelementptr` 返回字段指针
- VariableExpr: 返回变量的 alloca 地址

**右值**（值）: 用于计算

- 左值表达式需要 `load` 获取值
- 字面量直接使用常量

**控制标志**:

```cpp
generating_lvalue_ = true;  // 生成地址
// ... visit child ...
generating_lvalue_ = false;
```

## 基础表达式

### 1. 字面量 (`LiteralExpr`)

```cpp
void IRGenerator::visit(LiteralExpr *node)
```

**处理逻辑**:

#### 整数字面量

```rust
let x = 42;
```

生成: 直接使用常量 `"42"`

#### 布尔字面量

```rust
let flag = true;
```

生成: `"1"` (true) 或 `"0"` (false)

#### 字符字面量

```rust
let ch = 'A';
```

生成: ASCII 值 `"65"`

**实现**:

```cpp
if (node->type->kind == TypeKind::I32 || ...) {
    store_expr_result(node, node->value.lexeme);
} else if (node->type->kind == TypeKind::BOOL) {
    store_expr_result(node, node->value.lexeme == "true" ? "1" : "0");
}
```

### 2. 变量引用 (`VariableExpr`)

```cpp
void IRGenerator::visit(VariableExpr *node)
```

**查找变量**:

```cpp
auto var = value_manager_.lookup_variable(var_name);
```

**返回值决策**:

| 情况            | 返回        | 示例                     |
| --------------- | ----------- | ------------------------ |
| 左值生成        | 变量地址    | `arr[i] = 10` 中的 `arr` |
| 右值 + 非引用   | Load 后的值 | `let x = y;` 中的 `y`    |
| 右值 + 引用类型 | 指针本身    | `let r = &x;` 中的 `r`   |

**实现**:

```cpp
if (generating_lvalue_) {
    store_expr_result(node, var.alloca_name);
} else {
    if (is_reference_type) {
        // 引用类型: 指针本身就是值
        std::string ptr = emitter_.emit_load(var_type, var.alloca_name);
        store_expr_result(node, ptr);
    } else {
        // 非引用: load 实际值
        std::string value = emitter_.emit_load(var_type, var.alloca_name);
        store_expr_result(node, value);
    }
}
```

### 3. 一元运算 (`UnaryExpr`)

```cpp
void IRGenerator::visit(UnaryExpr *node)
```

**支持的运算符**:

#### 逻辑非 (`!`)

```rust
let result = !flag;
```

生成:

```llvm
%1 = xor i1 %flag, true
```

#### 算术负 (`-`)

```rust
let neg = -x;
```

生成:

```llvm
%1 = sub i32 0, %x
```

#### 解引用 (`*`)

```rust
let value = *ptr;
```

生成:

```llvm
%1 = load i32, i32* %ptr
```

**实现**:

```cpp
node->operand->accept(this);
std::string operand = get_expr_result(node->operand.get());

if (node->op.type == TokenType::BANG) {
    result = emitter_.emit_xor("i1", operand, "true");
} else if (node->op.type == TokenType::MINUS) {
    result = emitter_.emit_sub(type_str, "0", operand);
} else if (node->op.type == TokenType::STAR) {
    result = emitter_.emit_load(type_str, operand);
}
```

### 4. 二元运算 (`BinaryExpr`)

```cpp
void IRGenerator::visit(BinaryExpr *node)
```

**运算符分类**:

#### 算术运算

| 运算符 | LLVM 指令     | 示例                   |
| ------ | ------------- | ---------------------- |
| `+`    | `add`         | `%1 = add i32 %a, %b`  |
| `-`    | `sub`         | `%1 = sub i32 %a, %b`  |
| `*`    | `mul`         | `%1 = mul i32 %a, %b`  |
| `/`    | `sdiv`/`udiv` | `%1 = sdiv i32 %a, %b` |
| `%`    | `srem`/`urem` | `%1 = srem i32 %a, %b` |

**有符号 vs 无符号**:

```cpp
bool is_unsigned = (left_type->kind == TypeKind::U32 ||
                    left_type->kind == TypeKind::USIZE);

if (node->op.type == TokenType::SLASH) {
    result = is_unsigned ? emitter_.emit_udiv(...)
                         : emitter_.emit_sdiv(...);
}
```

#### 位运算

| 运算符 | LLVM 指令     |
| ------ | ------------- |
| `&`    | `and`         |
| `\|`   | `or`          |
| `^`    | `xor`         |
| `<<`   | `shl`         |
| `>>`   | `ashr`/`lshr` |

**移位运算**:

```rust
let result = x << 3;  // 左移3位
```

生成:

```llvm
%1 = shl i32 %x, 3
```

**注意**: 右移根据类型选择算术移位（`ashr`）或逻辑移位（`lshr`）

#### 比较运算

| 运算符 | 有符号     | 无符号     |
| ------ | ---------- | ---------- |
| `==`   | `icmp eq`  | `icmp eq`  |
| `!=`   | `icmp ne`  | `icmp ne`  |
| `<`    | `icmp slt` | `icmp ult` |
| `<=`   | `icmp sle` | `icmp ule` |
| `>`    | `icmp sgt` | `icmp ugt` |
| `>=`   | `icmp sge` | `icmp uge` |

**实现**:

```cpp
std::string cmp_predicate;
if (is_unsigned) {
    cmp_predicate = (op == "<") ? "ult" : "slt";
}
result = emitter_.emit_icmp(cmp_predicate, type_str, left, right);
```

#### 逻辑运算

**短路求值**: `&&` 和 `||` 使用条件分支实现

```rust
let result = a && b;
```

生成:

```llvm
  br i1 %a, label %and_rhs, label %and_end
and_rhs:
  %b_val = ...
  br label %and_end
and_end:
  %result = phi i1 [false, %entry], [%b_val, %and_rhs]
```

**优点**: 如果 `a` 为 false，不会求值 `b`

### 5. 赋值 (`AssignmentExpr`)

```cpp
void IRGenerator::visit(AssignmentExpr *node)
```

**流程**:

1. 生成目标的**地址**（左值）
2. 生成源值（右值）
3. Store 或 Memcpy

```cpp
generating_lvalue_ = true;
node->target->accept(this);
generating_lvalue_ = false;
std::string target_addr = get_expr_result(node->target.get());

node->value->accept(this);
std::string value = get_expr_result(node->value.get());

if (is_aggregate) {
    size_t size = get_type_size(node->value->type.get());
    emitter_.emit_memcpy(target_addr, value, size, ptr_type);
} else {
    emitter_.emit_store(type_str, value, target_addr);
}
```

### 6. 复合赋值 (`CompoundAssignmentExpr`)

```cpp
void IRGenerator::visit(CompoundAssignmentExpr *node)
```

**支持的运算符**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

**等价转换**:

```rust
x += 5;
// 等价于
x = x + 5;
```

**实现**:

```cpp
// 1. Load 当前值
generating_lvalue_ = true;
node->target->accept(this);
generating_lvalue_ = false;
std::string target_addr = get_expr_result(node->target.get());
std::string current_val = emitter_.emit_load(type_str, target_addr);

// 2. 计算新值
node->value->accept(this);
std::string rhs = get_expr_result(node->value.get());
std::string new_val = emit_binary_op(node->op, current_val, rhs);

// 3. Store 回去
emitter_.emit_store(type_str, new_val, target_addr);
```

### 7. 类型转换 (`AsExpr`)

```cpp
void IRGenerator::visit(AsExpr *node)
```

**转换类型**:

#### 整数扩展/截断

```rust
let x: i32 = 10;
let y: usize = x as usize;  // i32 → usize
```

32 位平台: `usize == i32`，无需转换

#### 符号扩展 vs 零扩展

```rust
let signed: i32 = -1;
let unsigned: u32 = signed as u32;  // 保持位模式
```

**实现**:

```cpp
node->expression->accept(this);
std::string value = get_expr_result(node->expression.get());

if (src_type_str == target_type_str) {
    // 相同类型，直接使用
    store_expr_result(node, value);
} else {
    // 需要转换 (当前32位平台大多不需要)
    store_expr_result(node, value);
}
```

### 8. 引用 (`ReferenceExpr`)

```cpp
void IRGenerator::visit(ReferenceExpr *node)
```

**获取地址**:

```rust
let x = 10;
let r = &x;  // r 是 x 的地址
```

**实现**:

```cpp
generating_lvalue_ = true;
node->expression->accept(this);
generating_lvalue_ = false;

std::string addr = get_expr_result(node->expression.get());
store_expr_result(node, addr);  // 引用就是地址
```

### 9. 函数调用 (`CallExpr`)

详见 [函数调用处理](./02_main_and_functions.md#函数调用)

**关键点**:

- 检查被调用函数是否使用 SRET
- 聚合类型参数传指针
- SRET 调用需要提供返回值地址

## 特殊表达式

### 1. 分组表达式 (`GroupingExpr`)

```rust
let x = (a + b) * c;
```

**实现**: 直接传递子表达式结果

```cpp
void IRGenerator::visit(GroupingExpr *node) {
    node->expression->accept(this);
    std::string value = get_expr_result(node->expression.get());
    store_expr_result(node, value);
}
```

### 2. Block 表达式 (`BlockExpr`)

```rust
let x = {
    let y = 10;
    y + 5
};  // x = 15
```

**实现**: 返回最终表达式的值

```cpp
void IRGenerator::visit(BlockExpr *node) {
    node->block->accept(this);

    if (node->block->final_expr.has_value()) {
        auto final_expr = node->block->final_expr.value();
        std::string result = get_expr_result(final_expr.get());
        store_expr_result(node, result);
    }
}
```

### 3. Unit 表达式 (`UnitExpr`)

```rust
let x = ();  // unit 类型
```

**实现**: 不存储任何结果（unit 类型无值）

```cpp
void IRGenerator::visit(UnitExpr *node) {
    // unit 类型无值，不需要生成 IR
}
```

## 优化技术

### 1. 常量折叠

**当前状态**: 部分实现

**示例**:

```rust
const MAX: usize = 100;
let arr = [0; MAX];  // MAX 在编译时求值
```

### 2. 短路求值

逻辑运算符 `&&` 和 `||` 使用条件跳转，避免不必要的计算。

### 3. 类型推导优化

无符号类型使用无符号指令（`udiv`, `urem`, `lshr`），性能更好。

## 已知限制

1. **浮点数**: 不支持 `f32`, `f64`
2. **字符串**: 不支持字符串字面量（除了内置函数的格式串）
3. **元组**: 不支持元组类型
4. **闭包**: 不支持闭包和函数指针

## 测试覆盖

- ✅ 所有算术运算
- ✅ 所有比较运算
- ✅ 所有位运算
- ✅ 逻辑短路求值
- ✅ 类型转换
- ✅ 赋值和复合赋值
- ✅ 引用和解引用

---

**参见**:

- [复杂表达式](./06_complex_expressions.md) - 数组和结构体
- [控制流](./05_control_flow.md) - if/while/loop 表达式
- [IR 发射器](./09_ir_emitter.md) - 底层指令发射
