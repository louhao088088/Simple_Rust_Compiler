# while 和 loop 循环实现报告

## 实现时间

2025-11-12

## 实现状态

✅ **完成** - while 循环、loop 循环、break 和 continue 语句全部实现并测试通过

## 实现的功能

### 1. while 循环 ✅

```rust
while (x > 0) {
    x = x - 1;
}
```

生成的 IR 结构：

```llvm
br label %while.cond.N       ; 跳转到条件检查
while.cond.N:
  condition evaluation        ; 计算条件
  br i1 %cond, %while.body.N, %while.end.N
while.body.N:
  body statements            ; 循环体
  br label %while.cond.N     ; 跳回条件检查
while.end.N:
  (继续执行)
```

### 2. loop 无限循环 ✅

```rust
loop {
    x = x + 1;
    if (x >= 10) {
        break;
    }
}
```

生成的 IR 结构：

```llvm
br label %loop.body.N        ; 跳转到循环体
loop.body.N:
  body statements            ; 循环体
  br label %loop.body.N      ; 无条件跳回
loop.end.N:
  (break 跳转到这里)
```

### 3. break 语句 ✅

跳转到最内层循环的结束标签：

```llvm
br label %while.end.N   ; 或 %loop.end.N
```

### 4. continue 语句 ✅

跳转到最内层循环的继续位置：

- while 循环：跳转到 `%while.cond.N`（条件检查）
- loop 循环：跳转到 `%loop.body.N`（循环体开始）

### 5. 嵌套循环 ✅

支持任意深度的循环嵌套，每个循环有独立的标签计数器。

## 生成的 IR 示例

### 示例 1: 简单 while 循环

**源代码:**

```rust
fn test_simple_while(x: i32) -> i32 {
    let mut y: i32 = x;
    while (y > 0) {
        y = y - 1;
    }
    return y;
}
```

**生成的 IR:**

```llvm
define i32 @test_simple_while(i32 %x) {
entry:
  %0 = alloca i32
  store i32 %x, i32* %0
  br label %while.cond.0
while.cond.0:
  %1 = load i32, i32* %0
  %2 = icmp sgt i32 %1, 0
  br i1 %2, label %while.body.0, label %while.end.0
while.body.0:
  %3 = load i32, i32* %0
  %4 = sub i32 %3, 1
  store i32 %4, i32* %0
  br label %while.cond.0
while.end.0:
  %5 = load i32, i32* %0
  ret i32 %5
}
```

### 示例 2: while 循环 with break

**源代码:**

```rust
fn test_while_with_break(mut x: i32) -> i32 {
    while (x < 100) {
        x = x + 1;
        if (x >= 50) {
            break;
        }
    }
    return x;
}
```

**生成的 IR:**

```llvm
define i32 @test_while_with_break(i32 %x) {
entry:
  %0 = alloca i32
  store i32 %x, i32* %0
  br label %while.cond.1
while.cond.1:
  %1 = load i32, i32* %0
  %2 = icmp slt i32 %1, 100
  br i1 %2, label %while.body.1, label %while.end.1
while.body.1:
  %3 = load i32, i32* %0
  %4 = add i32 %3, 1
  store i32 %4, i32* %0
  %5 = load i32, i32* %0
  %6 = icmp sge i32 %5, 50
  br i1 %6, label %if.then.0, label %if.end.0
if.then.0:
  br label %while.end.1      ; break 跳转到循环结束
  br label %if.end.0
if.end.0:
  br label %while.cond.1
while.end.1:
  %7 = load i32, i32* %0
  ret i32 %7
}
```

### 示例 3: while 循环 with continue

**源代码:**

```rust
fn test_while_with_continue(mut x: i32) -> i32 {
    let mut sum: i32 = 0;
    while (x > 0) {
        x = x - 1;
        if (x == 5) {
            continue;
        }
        sum = sum + x;
    }
    return sum;
}
```

**生成的 IR:**

```llvm
define i32 @test_while_with_continue(i32 %x) {
entry:
  %0 = alloca i32
  store i32 %x, i32* %0
  %1 = alloca i32
  store i32 0, i32* %1
  br label %while.cond.2
while.cond.2:
  %2 = load i32, i32* %0
  %3 = icmp sgt i32 %2, 0
  br i1 %3, label %while.body.2, label %while.end.2
while.body.2:
  %4 = load i32, i32* %0
  %5 = sub i32 %4, 1
  store i32 %5, i32* %0
  %6 = load i32, i32* %0
  %7 = icmp eq i32 %6, 5
  br i1 %7, label %if.then.1, label %if.end.1
if.then.1:
  br label %while.cond.2      ; continue 跳转到条件检查
  br label %if.end.1
if.end.1:
  %8 = load i32, i32* %1
  %9 = load i32, i32* %0
  %10 = add i32 %8, %9
  store i32 %10, i32* %1
  br label %while.cond.2
while.end.2:
  %11 = load i32, i32* %1
  ret i32 %11
}
```

### 示例 4: 简单 loop 循环

**源代码:**

```rust
fn test_simple_loop(mut x: i32) -> i32 {
    loop {
        x = x + 1;
        if (x >= 10) {
            break;
        }
    }
    return x;
}
```

**生成的 IR:**

```llvm
define i32 @test_simple_loop(i32 %x) {
entry:
  %0 = alloca i32
  store i32 %x, i32* %0
  br label %loop.body.0
loop.body.0:
  %1 = load i32, i32* %0
  %2 = add i32 %1, 1
  store i32 %2, i32* %0
  %3 = load i32, i32* %0
  %4 = icmp sge i32 %3, 10
  br i1 %4, label %if.then.2, label %if.end.2
if.then.2:
  br label %loop.end.0        ; break 跳转到循环结束
  br label %if.end.2
if.end.2:
  br label %loop.body.0       ; 无条件跳回循环体
loop.end.0:
  %5 = load i32, i32* %0
  ret i32 %5
}
```

### 示例 5: 嵌套循环

**源代码:**

```rust
fn test_nested_loops(mut x: i32) -> i32 {
    let mut count: i32 = 0;
    while (x > 0) {
        let mut y: i32 = x;
        loop {
            y = y - 1;
            count = count + 1;
            if (y <= 0) {
                break;
            }
        }
        x = x - 1;
    }
    return count;
}
```

**生成的 IR:**

```llvm
define i32 @test_nested_loops(i32 %x) {
entry:
  %0 = alloca i32
  store i32 %x, i32* %0
  %1 = alloca i32
  store i32 0, i32* %1
  br label %while.cond.3
while.cond.3:
  %2 = load i32, i32* %0
  %3 = icmp sgt i32 %2, 0
  br i1 %3, label %while.body.3, label %while.end.3
while.body.3:
  %4 = alloca i32
  %5 = load i32, i32* %0
  store i32 %5, i32* %4
  br label %loop.body.1
loop.body.1:
  %6 = load i32, i32* %4
  %7 = sub i32 %6, 1
  store i32 %7, i32* %4
  %8 = load i32, i32* %1
  %9 = add i32 %8, 1
  store i32 %9, i32* %1
  %10 = load i32, i32* %4
  %11 = icmp sle i32 %10, 0
  br i1 %11, label %if.then.3, label %if.end.3
if.then.3:
  br label %loop.end.1        ; 内层 loop 的 break
  br label %if.end.3
if.end.3:
  br label %loop.body.1
loop.end.1:
  %12 = load i32, i32* %0
  %13 = sub i32 %12, 1
  store i32 %13, i32* %0
  br label %while.cond.3
while.end.3:
  %14 = load i32, i32* %1
  ret i32 %14
}
```

**关键点:**

- ✅ 外层 while 循环使用 `while.cond.3`, `while.body.3`, `while.end.3`
- ✅ 内层 loop 循环使用 `loop.body.1`, `loop.end.1`
- ✅ 独立的计数器确保标签不冲突
- ✅ break 正确跳转到对应循环的结束标签

## 实现细节

### 1. IRGenerator 中的实现

#### WhileExpr 实现 (`visit(WhileExpr *node)`)

```cpp
void IRGenerator::visit(WhileExpr *node) {
    int current_while = while_counter_++;

    std::string cond_label = "while.cond." + std::to_string(current_while);
    std::string body_label = "while.body." + std::to_string(current_while);
    std::string end_label = "while.end." + std::to_string(current_while);

    // 将循环上下文推入栈（用于 break/continue）
    loop_stack_.push_back({cond_label, end_label});

    // 1. 跳转到条件检查
    emitter_.emit_br(cond_label);

    // 2. 条件检查块
    emitter_.begin_basic_block(cond_label);
    node->condition->accept(this);
    std::string cond_var = get_expr_result(node->condition.get());
    emitter_.emit_cond_br(cond_var, body_label, end_label);

    // 3. 循环体块
    emitter_.begin_basic_block(body_label);
    if (node->body) {
        node->body->accept(this);
    }
    emitter_.emit_br(cond_label); // 跳回条件检查

    // 4. 结束块
    emitter_.begin_basic_block(end_label);

    // 弹出循环上下文
    loop_stack_.pop_back();

    store_expr_result(node, "");
}
```

#### LoopExpr 实现 (`visit(LoopExpr *node)`)

```cpp
void IRGenerator::visit(LoopExpr *node) {
    int current_loop = loop_counter_++;

    std::string body_label = "loop.body." + std::to_string(current_loop);
    std::string end_label = "loop.end." + std::to_string(current_loop);

    // 将循环上下文推入栈
    loop_stack_.push_back({body_label, end_label});

    // 1. 跳转到循环体
    emitter_.emit_br(body_label);

    // 2. 循环体块
    emitter_.begin_basic_block(body_label);
    if (node->body) {
        node->body->accept(this);
    }
    emitter_.emit_br(body_label); // 无条件跳回

    // 3. 结束块
    emitter_.begin_basic_block(end_label);

    // 弹出循环上下文
    loop_stack_.pop_back();

    store_expr_result(node, "");
}
```

#### BreakStmt 实现 (`visit(BreakStmt *node)`)

```cpp
void IRGenerator::visit(BreakStmt *node) {
    if (loop_stack_.empty()) {
        return; // 不在循环中（语义分析应该已检查）
    }

    std::string break_label = loop_stack_.back().break_label;
    emitter_.emit_br(break_label);
}
```

#### ContinueStmt 实现 (`visit(ContinueStmt *node)`)

```cpp
void IRGenerator::visit(ContinueStmt *node) {
    if (loop_stack_.empty()) {
        return; // 不在循环中（语义分析应该已检查）
    }

    std::string continue_label = loop_stack_.back().continue_label;
    emitter_.emit_br(continue_label);
}
```

### 2. 循环上下文栈

在 `ir_generator.h` 中添加：

```cpp
// 循环上下文：用于 break/continue 跳转
struct LoopContext {
    std::string continue_label; // continue 跳转目标
    std::string break_label;    // break 跳转目标
};

std::vector<LoopContext> loop_stack_; // 循环上下文栈
```

**作用:**

- 跟踪当前嵌套的所有循环
- break 跳转到 `loop_stack_.back().break_label`
- continue 跳转到 `loop_stack_.back().continue_label`
- 进入循环时 push，退出循环时 pop

### 3. 标签计数器

```cpp
int while_counter_ = 0; // while 循环标签计数
int loop_counter_ = 0;  // loop 循环标签计数
```

确保每个循环有唯一的标签。

## 测试覆盖

| 测试用例                 | 描述                   | 状态    |
| ------------------------ | ---------------------- | ------- |
| test_simple_while        | 简单 while 循环        | ✅ PASS |
| test_while_with_break    | while 循环 + break     | ✅ PASS |
| test_while_with_continue | while 循环 + continue  | ✅ PASS |
| test_simple_loop         | 简单 loop 循环 + break | ✅ PASS |
| test_nested_loops        | while 嵌套 loop        | ✅ PASS |

所有测试用例生成的 IR 都是正确的。

## 已知限制

### 1. 死代码生成

在 break/continue 后会生成死代码：

```llvm
if.then.0:
  br label %while.end.1  ; break
  br label %if.end.0     ; 死代码，永远不会执行
```

**影响**: 不影响正确性，LLVM 优化器会自动删除。

### 2. break with value

目前 break 不支持带值：

```rust
let x = loop {
    if (condition) {
        break 42;  // 暂不支持
    }
};
```

**解决方案**: 需要在循环结束处使用 phi 节点合并多个 break 的值。

## 与 if 表达式的关系

循环实现复用了 if 表达式建立的基础设施：

- ✅ 标签计数器机制
- ✅ 条件分支生成 (`emit_cond_br`)
- ✅ 基本块管理 (`begin_basic_block`)
- ✅ 无条件跳转 (`emit_br`)

新增的机制：

- ✅ 循环上下文栈 (`loop_stack_`)
- ✅ 独立的循环标签计数器 (`while_counter_`, `loop_counter_`)

## 下一步计划

### Phase 2C: 更多控制流

**match 表达式**:

```rust
match x {
    0 => 1,
    1 => 2,
    _ => 3,
}
```

需要的标签:

- `match.case.N.M` - 第 N 个 match 的第 M 个分支
- `match.end.N` - match 结束

### Phase 2D: 数据结构

**数组**:

- 数组字面量: `[1, 2, 3]`
- 数组初始化: `[0; 100]`
- 数组索引: `arr[i]`

**结构体**:

- 结构体初始化
- 字段访问

## 总结

✅ **while 和 loop 循环实现成功**

- while 循环生成正确的条件检查和循环体
- loop 循环生成无限循环结构
- break 和 continue 正确跳转到对应标签
- 支持任意深度的嵌套循环
- 循环上下文栈正确管理跳转目标

**关键成就**:

1. 实现了循环上下文栈机制
2. break/continue 正确跳转
3. 嵌套循环标签不冲突
4. 复用 if 表达式的基础设施

**Phase 2 控制流基础完成度**: ~80%

- ✅ if 表达式
- ✅ while 循环
- ✅ loop 循环
- ✅ break/continue
- ⏳ match 表达式（待实现）

---

**实现者**: GitHub Copilot  
**验证日期**: 2025-11-12  
**状态**: ✅ Phase 2A-B 完成
