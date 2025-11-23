# 语句生成

本模块实现语句的 IR 生成，包括变量声明、return、break/continue 等。

## 文件位置

`src/ir/ir_generator_statements.cpp`

## 核心语句

### 1. Block 语句 (`BlockStmt`)

```cpp
void IRGenerator::visit(BlockStmt *node)
```

**作用域管理**:

```cpp
value_manager_.enter_scope();
// ... 处理语句 ...
value_manager_.exit_scope();
```

**关键修复**: 移除了错误的 shadowing 优化

**之前的错误**:

```cpp
// 错误: 跳过第一个 let，导致副作用丢失
if (next_let shadows current_let) {
    skip current_let;  // ❌ 副作用也被跳过
}
```

**修复后**:

```cpp
// 正确: 执行所有语句，包括副作用
for (auto &stmt : node->statements) {
    if (current_block_terminated_) break;
    stmt->accept(this);  // ✅ 保证副作用执行
}
```

**测试用例** (comprehensive19):

```rust
let new_node: i32 = allocate_node(pool, data);  // 调用1
let new_node: i32 = allocate_node(pool, data);  // 调用2
// 两次调用都必须执行！
```

### 2. Let 语句 (`LetStmt`)

```cpp
void IRGenerator::visit(LetStmt *node)
```

**处理流程**:

#### 1. 类型推导

```cpp
std::shared_ptr<Type> var_type;
if (node->type_annotation.has_value()) {
    var_type = node->type_annotation.value()->resolved_type;
} else if (node->initializer.has_value()) {
    var_type = node->initializer.value()->type;
}
```

#### 2. 引用类型特殊处理

```rust
let r: &i32 = &x;
```

**不创建 alloca**:

```cpp
if (is_reference && node->initializer.has_value()) {
    init_expr->accept(this);
    std::string addr = get_expr_result(init_expr.get());
    value_manager_.define_variable(var_name, addr, type_str, is_mutable);
    return;  // 直接返回，不创建 alloca
}
```

#### 3. 聚合类型优化

**目标地址传递**:

```rust
let arr = [0; 100];  // 数组初始化器
```

```cpp
std::string alloca = emitter_.emit_alloca("[100 x i32]");
set_target_address(alloca);  // 提供目标
init_expr->accept(this);     // 原地初始化
take_target_address();       // 清除
```

**避免临时拷贝**:

- 数组字面量: `[1, 2, 3]`
- 数组初始化器: `[0; 100]`
- 结构体初始化器: `Point { x: 1, y: 2 }`
- 返回聚合类型的函数调用

```cpp
bool is_aggregate_returns_pointer =
    (is_literal || is_call_ret_aggregate);

if (is_aggregate_returns_pointer) {
    init_expr->accept(this);
    alloca_name = get_expr_result(init_expr.get());
    // 直接使用表达式返回的指针
} else {
    alloca_name = emitter_.emit_alloca(type_str);
    // ... 初始化 ...
}
```

#### 4. 变量注册

```cpp
value_manager_.define_variable(var_name, alloca_name,
                                ptr_type_str, is_mutable);
```

### 3. Return 语句 (`ReturnStmt`)

```cpp
void IRGenerator::visit(ReturnStmt *node)
```

**SRET 处理**:

```cpp
if (current_function_uses_sret_) {
    // TODO: 拷贝返回值到 sret_ptr
    emitter_.emit_ret_void();
} else {
    std::string type_str = type_mapper_.map(return_expr->type.get());
    emitter_.emit_ret(type_str, return_value);
}
current_block_terminated_ = true;
```

**聚合类型返回**:

```cpp
if (is_aggregate) {
    // 需要 load 整个聚合值
    return_value = emitter_.emit_load(type_str, return_value);
}
```

### 4. Break/Continue 语句

```cpp
void IRGenerator::visit(BreakStmt *node)
void IRGenerator::visit(ContinueStmt *node)
```

**实现**:

```cpp
if (!loop_stack_.empty()) {
    std::string target = loop_stack_.back().break_label;  // 或 continue_label
    emitter_.emit_br(target);
    current_block_terminated_ = true;
}
```

**循环栈管理**:

```cpp
// 进入循环
loop_stack_.push_back({continue_label, break_label});
// ... 生成循环体 ...
loop_stack_.pop_back();
```

### 5. 表达式语句 (`ExprStmt`)

```cpp
void IRGenerator::visit(ExprStmt *node)
```

**简单实现**:

```cpp
if (node->expression) {
    node->expression->accept(this);
    // 结果可能未使用，但副作用已执行
}
```

**示例**:

```rust
foo();  // 函数调用语句
x += 1;  // 复合赋值语句
```

## 关键修复

### 副作用保证问题（已修复）

**问题**: BlockStmt 中的 shadowing 优化跳过了被 shadow 的 let 语句

**影响**:

```rust
let x = func1();  // ❌ 被跳过，func1() 未调用
let x = func2();  // ✅ 执行
```

**修复**: 移除优化，保证所有语句执行

**测试**: comprehensive19 - 内存分配器测试

- 修复前: 100 个节点（只调用一次 allocate_node）
- 修复后: 200 个节点（调用两次）

## 优化技术

### 1. 原地初始化

避免为聚合类型创建临时变量：

```rust
let arr = [0; 1000];  // 直接在 alloca 上初始化
```

**节省**: 不需要临时 alloca + memcpy

### 2. 引用优化

引用类型不创建 alloca，直接使用指针：

```rust
let r = &x;  // r 直接指向 x 的 alloca
```

### 3. 终止检测

检测到 return/break/continue 后，标记块已终止，跳过后续死代码生成。

## 测试覆盖

- ✅ 基础变量声明
- ✅ 引用声明
- ✅ 数组/结构体声明
- ✅ shadowing（重复声明）
- ✅ return 语句
- ✅ break/continue
- ✅ 嵌套作用域
- ✅ 副作用保证

---

**参见**:

- [表达式生成](./03_expressions.md)
- [控制流](./05_control_flow.md)
- [值管理](./11_value_manager.md)
