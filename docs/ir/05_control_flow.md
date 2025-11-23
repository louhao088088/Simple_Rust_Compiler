# 控制流生成

本模块实现控制流表达式的 IR 生成：if、while、loop。

## 文件位置

`src/ir/ir_generator_control_flow.cpp`

## If 表达式

### 基本结构

```rust
let x = if condition { 10 } else { 20 };
```

**生成的 IR**:

```llvm
  ; 计算条件
  %cond = ...
  br i1 %cond, label %if.then.0, label %if.else.0

if.then.0:
  ; then 分支
  br label %if.end.0

if.else.0:
  ; else 分支
  br label %if.end.0

if.end.0:
  ; PHI 节点合并结果
  %result = phi i32 [10, %if.then.0], [20, %if.else.0]
```

### 实现细节

```cpp
void IRGenerator::visit(IfExpr *node)
```

#### 1. 标签生成

```cpp
int current_if = if_counter_++;
std::string then_label = "if.then." + std::to_string(current_if);
std::string else_label = "if.else." + std::to_string(current_if);
std::string end_label = "if.end." + std::to_string(current_if);
```

#### 2. 条件分支

```cpp
node->condition->accept(this);
std::string cond_var = get_expr_result(node->condition.get());

if (node->else_branch.has_value()) {
    emitter_.emit_cond_br(cond_var, then_label, else_label);
} else {
    emitter_.emit_cond_br(cond_var, then_label, end_label);
}
```

#### 3. Then 分支

```cpp
begin_block(then_label);
current_block_terminated_ = false;

node->then_branch->accept(this);
bool then_terminated = current_block_terminated_;
std::string then_result = get_expr_result(node->then_branch.get());
std::string then_pred_block = current_block_label_;

if (!then_terminated) {
    emitter_.emit_br(end_label);
}
```

#### 4. Else 分支（可选）

```cpp
if (node->else_branch.has_value()) {
    begin_block(else_label);
    current_block_terminated_ = false;

    node->else_branch.value()->accept(this);
    else_terminated = current_block_terminated_;
    // ...
}
```

#### 5. PHI 节点生成

**条件**: 两个分支都有返回值且至少一个未终止

```cpp
if (need_end_block && then_has_value && else_has_value &&
    (!then_terminated || !else_terminated)) {

    std::vector<std::pair<std::string, std::string>> phi_incoming;

    if (!then_terminated) {
        phi_incoming.push_back({then_result, then_pred_block});
    }
    if (!else_terminated) {
        phi_incoming.push_back({else_result, else_pred_block});
    }

    std::string phi_result = emitter_.emit_phi(result_type, phi_incoming);
    store_expr_result(node, phi_result);
}
```

**聚合类型处理**:

```cpp
bool is_aggregate = (node->type->kind == TypeKind::ARRAY ||
                     node->type->kind == TypeKind::STRUCT);
if (is_aggregate) {
    result_type += "*";  // PHI 传递指针
}
```

### 无 else 分支

```rust
if condition {
    do_something();
}
// 继续执行
```

**生成**:

```llvm
  br i1 %cond, label %if.then.0, label %if.end.0
if.then.0:
  ; ...
  br label %if.end.0
if.end.0:
  ; 继续
```

**特点**: 没有 PHI 节点（无返回值）

## While 循环

### 基本结构

```rust
while condition {
    body();
}
```

**生成的 IR**:

```llvm
  br label %while.cond.0
while.cond.0:
  %cond = ...
  br i1 %cond, label %while.body.0, label %while.end.0
while.body.0:
  ; 循环体
  br label %while.cond.0
while.end.0:
  ; 循环后
```

### 实现

```cpp
void IRGenerator::visit(WhileExpr *node)
```

#### 1. 标签生成

```cpp
int current_loop = loop_counter_++;
std::string cond_label = "while.cond." + std::to_string(current_loop);
std::string body_label = "while.body." + std::to_string(current_loop);
std::string end_label = "while.end." + std::to_string(current_loop);
```

#### 2. 循环栈

```cpp
loop_stack_.push_back({cond_label, end_label});
// ... 生成循环 ...
loop_stack_.pop_back();
```

**用途**: 支持 `break` 和 `continue`

#### 3. 条件块

```cpp
emitter_.emit_br(cond_label);
begin_block(cond_label);

node->condition->accept(this);
std::string cond_var = get_expr_result(node->condition.get());
emitter_.emit_cond_br(cond_var, body_label, end_label);
```

#### 4. 循环体

```cpp
begin_block(body_label);
current_block_terminated_ = false;

node->body->accept(this);

if (!current_block_terminated_) {
    emitter_.emit_br(cond_label);  // 回到条件判断
}
```

#### 5. 结束块

```cpp
begin_block(end_label);
current_block_terminated_ = false;
```

## Loop 循环（无限循环）

### 基本结构

```rust
loop {
    if done { break; }
    body();
}
```

**生成的 IR**:

```llvm
  br label %loop.body.0
loop.body.0:
  ; 循环体
  br i1 %done, label %loop.end.0, label %loop.body.0
loop.end.0:
  ; 循环后
```

### 实现

```cpp
void IRGenerator::visit(LoopExpr *node)
```

**类似 while，但没有条件检查**:

```cpp
std::string body_label = "loop.body." + std::to_string(loop_counter_);
std::string end_label = "loop.end." + std::to_string(loop_counter_++);

loop_stack_.push_back({body_label, end_label});

emitter_.emit_br(body_label);
begin_block(body_label);

node->body->accept(this);

if (!current_block_terminated_) {
    emitter_.emit_br(body_label);  // 无条件回跳
}

loop_stack_.pop_back();
begin_block(end_label);
```

## Break 和 Continue

### Break

```rust
loop {
    if condition { break; }
}
```

**生成**:

```llvm
  br i1 %condition, label %break_target, label %continue_exec
break_target:
  br label %loop.end.0  ; 跳出循环
```

### Continue

```rust
while condition {
    if skip { continue; }
    process();
}
```

**生成**:

```llvm
  br i1 %skip, label %continue_target, label %process
continue_target:
  br label %while.cond.0  ; 回到条件判断
```

## 关键特性

### 1. 终止检测

每个分支/循环体后检查 `current_block_terminated_`：

- 避免生成不可达代码
- 正确处理 return/break/continue

### 2. PHI 节点优化

**只在需要时生成**:

- 两个分支都有值
- 至少一个分支未终止
- 类型不是 unit

### 3. 聚合类型支持

If 表达式返回数组/结构体时，PHI 传递指针：

```rust
let arr = if flag { arr1 } else { arr2 };
```

```llvm
%result = phi [100 x i32]* [%arr1_ptr, %then], [%arr2_ptr, %else]
```

### 4. 循环栈管理

支持嵌套循环中的 break/continue：

```rust
'outer: loop {
    loop {
        if cond { break 'outer; }  // 跳出外层
    }
}
```

**当前限制**: 不支持循环标签，只能 break/continue 最内层循环

## 测试覆盖

- ✅ if-else 表达式
- ✅ if 无 else
- ✅ if 返回值
- ✅ if 返回聚合类型
- ✅ 嵌套 if
- ✅ while 循环
- ✅ loop 无限循环
- ✅ break/continue
- ✅ 嵌套循环
- ✅ 循环中的 return

## 已知问题

### 1. 无循环标签

不支持 `'label: loop` 语法，无法指定 break/continue 目标。

### 2. 无 do-while

不支持 `do { } while (cond);` 语法。

---

**参见**:

- [语句生成](./04_statements.md) - break/continue 实现
- [表达式生成](./03_expressions.md) - 条件表达式
- [IR 发射器](./09_ir_emitter.md) - 分支指令
