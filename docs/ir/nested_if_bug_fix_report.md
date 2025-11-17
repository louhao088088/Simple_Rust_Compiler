# 嵌套 If 语句 PHI 节点 Bug 修复报告

## 问题描述

在测试嵌套 if 语句时，发现生成的 LLVM IR 中 PHI 节点的前驱块标签错误，导致 IR 验证失败。

### 问题代码

```rust
fn classify_number(n: i32) -> i32 {
    if (n > 0) {
        if (n > 10) {
            3  // 大数
        } else {
            2  // 小正数
        }
    } else {
        if (n == 0) {
            1  // 零
        } else {
            0  // 负数
        }
    }
}
```

### 错误的 IR

```llvm
if.end.0:
  %9 = phi i32 [%5, %if.then.0], [%8, %if.else.0]  ; ❌ 错误的前驱块
  ret i32 %9
```

### LLVM 错误信息

```
PHI node entries do not match predecessors!
  %9 = phi i32 [ %5, %if.then.0 ], [ %8, %if.else.0 ]
label %if.then.0
label %if.end.1  ; ← 实际的前驱块是 if.end.1 和 if.end.2
```

## 问题分析

### 根本原因

在处理嵌套 if 表达式时，外层 if 的 PHI 节点使用了**分支开始时的块标签**（`if.then.0`, `if.else.0`），而不是**跳转到 end 块之前的最后一个块标签**（`if.end.1`, `if.end.2`）。

### 控制流图

```
entry
  ├─ if.then.0 (外层then)
  │   └─ if (n > 10)
  │       ├─ if.then.1 → if.end.1
  │       └─ if.else.1 → if.end.1
  │   if.end.1 → if.end.0  ← 实际前驱
  │
  └─ if.else.0 (外层else)
      └─ if (n == 0)
          ├─ if.then.2 → if.end.2
          └─ if.else.2 → if.end.2
      if.end.2 → if.end.0  ← 实际前驱
```

### 问题代码

```cpp
// ❌ 错误的实现
emitter_.begin_basic_block(then_label);  // then_label = "if.then.0"
node->then_branch->accept(this);         // 执行内层if，当前块变为 "if.end.1"

// 使用初始的标签，而不是当前块
phi_incoming.push_back({then_result, then_label});  // ❌ 应该用 "if.end.1"
```

## 解决方案

### 修复策略

在发出`br`指令跳转到 end 块之前，**记录当前基本块的标签**，用于后续 PHI 节点的前驱块。

### 实现步骤

#### 1. 添加成员变量

在`IRGenerator`类中添加当前块标签跟踪：

```cpp
// ir_generator.h
class IRGenerator {
private:
    std::string current_block_label_;  // 当前基本块的标签名
};
```

#### 2. 创建辅助方法

创建`begin_block`方法，同时更新块标签和调用 emitter：

```cpp
// ir_generator.cpp
void IRGenerator::begin_block(const std::string &label) {
    emitter_.begin_basic_block(label);
    current_block_label_ = label;  // 自动更新当前块标签
}
```

#### 3. 修复 if 表达式处理

在处理 then/else 分支时记录实际的前驱块：

```cpp
void IRGenerator::visit(IfExpr *node) {
    // ... 生成条件判断 ...

    // 3. 生成 then 分支
    begin_block(then_label);
    current_block_terminated_ = false;

    node->then_branch->accept(this);  // 可能修改 current_block_label_

    // ✅ 记录跳转前的实际块标签
    std::string then_pred_block = current_block_label_;

    if (!then_terminated) {
        emitter_.emit_br(end_label);
    }

    // 4. 生成 else 分支（类似处理）
    std::string else_pred_block;
    if (node->else_branch.has_value()) {
        begin_block(else_label);
        node->else_branch.value()->accept(this);
        else_pred_block = current_block_label_;  // ✅ 记录实际块
        // ...
    }

    // 5. 生成 PHI 节点
    if (need_phi) {
        phi_incoming.push_back({then_result, then_pred_block});  // ✅ 正确
        phi_incoming.push_back({else_result, else_pred_block});  // ✅ 正确
    }
}
```

### 正确的 IR

修复后生成的 IR：

```llvm
if.end.0:
  %9 = phi i32 [%5, %if.end.1], [%8, %if.end.2]  ; ✅ 正确的前驱块
  ret i32 %9
```

## 验证结果

### 修复前

```bash
❌ FAIL: test_nested_if.rs
PHI node entries do not match predecessors!
```

### 修复后

```bash
✅ PASS: test_nested_if.rs
[1/6] IR生成 ✓
[2/6] 语法验证 ✓
[3/6] 优化 ✓
[4/6] 解释执行 ✓ (返回值: 2)
[5/6] 编译汇编 ✓
[6/6] 原生执行 ✓ (返回值: 2)
```

### 完整测试套件

所有 12 个语义测试全部通过：

```
✅ test_simple_return.rs       (42)
✅ test_fibonacci.rs          (55)
✅ test_array_sum.rs          (15)
✅ test_struct_method.rs      (25)
✅ test_gcd.rs                (12)
✅ test_factorial.rs          (120)
✅ test_2d_array.rs           (14)
✅ test_struct_fields.rs      (17)
✅ test_methods.rs            (50)
✅ test_nested_if.rs          (2) ← 修复的测试
✅ test_impl_associated_fn.rs (7)
✅ test_impl_methods.rs       (32)

总计: 12/12 (100%)
```

## 技术要点

### PHI 节点的前驱要求

LLVM IR 要求 PHI 节点的前驱块必须满足：

1. **前驱块必须真实存在**于 CFG 中
2. **前驱块必须有直接跳转**到当前块的 br 指令
3. **PHI 值必须在前驱块中 dominate**使用点

### 嵌套表达式处理原则

处理嵌套结构时必须注意：

1. **内层表达式会修改全局状态**（如 current*block_label*）
2. **必须在 accept 前后记录关键状态**
3. **不能假设 accept 后状态不变**

### 基本块标签管理

良好的实践：

- ✅ 使用辅助方法`begin_block`统一管理
- ✅ 在 emit_br 之前记录前驱块
- ✅ 为每个 if/while 使用唯一计数器
- ❌ 不要直接调用`emitter_.begin_basic_block`

## 影响范围

### 受影响的功能

- ✅ 嵌套 if 表达式
- ✅ if 表达式作为值（返回类型非 unit）
- ✅ 多层嵌套的控制流

### 不受影响的功能

- ✅ 简单 if 语句（无嵌套）
- ✅ while/loop 循环
- ✅ 函数调用、数组、结构体等

## 经验教训

1. **仔细阅读错误信息**：LLVM 的错误信息非常详细，明确指出了前驱块不匹配
2. **理解控制流图**：绘制 CFG 有助于理解块之间的跳转关系
3. **状态追踪很重要**：在递归 visitor 中必须谨慎管理全局状态
4. **测试覆盖嵌套场景**：简单测试可能隐藏嵌套结构的 bug

## 相关文件

- `src/ir/ir_generator.h` - 添加`current_block_label_`成员
- `src/ir/ir_generator.cpp` - 修复 PHI 节点生成逻辑
- `test1/ir/verify/test_nested_if.rs` - 触发 bug 的测试用例
- `scripts/test_ir_semantics.sh` - 语义验证测试套件

## 参考

- [LLVM Language Reference - PHI Instruction](https://llvm.org/docs/LangRef.html#phi-instruction)
- [SSA Form and PHI Nodes](https://en.wikipedia.org/wiki/Static_single_assignment_form)
- `docs/ir/IR验证流程.md` - IR 验证文档
