# 嵌套函数支持机制

## 概述

IRGenerator 支持在函数体内定义嵌套函数（Nested Functions），并在生成 LLVM IR 时将其**提升为顶层函数**。这意味着：

- Rust 源码层面允许嵌套函数语法
- IR 生成时会将所有嵌套函数转换为模块级函数
- 不保留嵌套作用域的捕获语义（类似 C 的嵌套函数提升）

本文档详细说明实现机制、状态管理和潜在限制。

---

## 核心机制

### 1. 检测与收集阶段

**触发点**：`visit(ItemStmt*)` 在处理函数体内的语句时
**位置**：`src/ir/ir_generator_statements.cpp:276-285`

```cpp
// 处理嵌套函数定义
if (auto fn_decl = dynamic_cast<FnDecl *>(node->item.get())) {
    if (inside_function_body_) {
        // 在函数体内定义的函数：收集起来稍后提升为顶层函数
        nested_functions_.push_back(fn_decl);
    } else {
        // 顶层函数：直接处理
        visit_function_decl(fn_decl);
    }
    return;
}
```

**关键状态**：

- `inside_function_body_` (bool)：标记当前是否在函数体内
- `nested_functions_` (vector<FnDecl\*>)：当前函数内收集的嵌套函数队列

### 2. 延迟生成阶段

**触发点**：外层函数处理完成后
**位置**：`src/ir/ir_generator_main.cpp:296-299`

```cpp
// 12. 处理在此函数体内定义的嵌套函数（提升为顶层函数）
for (FnDecl *nested_fn : nested_functions_) {
    visit_function_decl(nested_fn);
}
```

**工作流程**：

1. 外层函数体生成完毕（emit `}`）
2. 遍历 `nested_functions_` 队列
3. 递归调用 `visit_function_decl(nested_fn)`
4. 嵌套函数在 IR 中作为独立的顶层函数输出

### 3. 多层嵌套支持

**状态保护**：`visit_function_decl` 入口处理多层嵌套
**位置**：`src/ir/ir_generator_main.cpp:70-77, 301-302`

```cpp
void IRGenerator::visit_function_decl(FnDecl *node) {
    // 保存当前的嵌套函数列表（用于处理多层嵌套）
    std::vector<FnDecl *> outer_nested_functions = std::move(nested_functions_);
    nested_functions_.clear();

    // 标记进入函数体
    bool was_inside = inside_function_body_;
    inside_function_body_ = true;

    // ... 函数体生成 ...

    // 11. 恢复嵌套函数状态
    inside_function_body_ = was_inside;

    // 12. 处理在此函数体内定义的嵌套函数（提升为顶层函数）
    for (FnDecl *nested_fn : nested_functions_) {
        visit_function_decl(nested_fn);
    }

    // 13. 恢复外层的嵌套函数列表
    nested_functions_ = std::move(outer_nested_functions);
}
```

**保护机制**：

- 进入函数前保存外层的 `nested_functions_` 到 `outer_nested_functions`
- 清空当前 `nested_functions_` 以收集内层函数
- 退出时先处理当前层嵌套函数，再恢复外层列表

---

## 示例转换

### 输入 Rust 代码

```rust
fn outer(x: i32) -> i32 {
    fn inner(y: i32) -> i32 {
        y * 2
    }

    let result = inner(x);
    result + 1
}
```

### 生成的 LLVM IR（简化）

```llvm
; 外层函数
define i32 @outer(i32 %x) {
bb.entry:
  %x.addr = alloca i32
  store i32 %x, ptr %x.addr
  %result.addr = alloca i32
  %0 = load i32, ptr %x.addr
  %1 = call i32 @inner(i32 %0)    ; 调用提升后的顶层函数
  store i32 %1, ptr %result.addr
  %2 = load i32, ptr %result.addr
  %3 = add i32 %2, 1
  ret i32 %3
}

; 提升后的嵌套函数（作为顶层函数）
define i32 @inner(i32 %y) {
bb.entry:
  %y.addr = alloca i32
  store i32 %y, ptr %y.addr
  %0 = load i32, ptr %y.addr
  %1 = mul i32 %0, 2
  ret i32 %1
}
```

---

## 状态管理字段

### 相关字段（`src/ir/ir_generator.h`）

```cpp
// ========== 嵌套函数支持 ==========

/**
 * 嵌套函数队列：存储在函数体内定义的函数
 * 这些函数会在当前函数生成完成后被提升为顶层函数
 */
std::vector<FnDecl *> nested_functions_;

/**
 * 标记当前是否在函数体内（用于检测嵌套函数）
 */
bool inside_function_body_ = false;

/**
 * Local struct队列：存储在函数体内定义的struct
 * 这些struct的类型定义需要在模块顶部生成
 * 使用set避免重复收集
 */
std::set<StructDecl *> local_structs_set_;
```

### 状态生命周期

| 阶段             | `inside_function_body_` | `nested_functions_` | 操作         |
| ---------------- | ----------------------- | ------------------- | ------------ |
| 进入外层函数     | `true`                  | 空列表              | 保存外层列表 |
| 遇到嵌套函数声明 | `true`                  | 收集 `FnDecl*`      | 加入队列     |
| 外层函数体完成   | 恢复                    | 当前层列表          | 递归生成     |
| 处理嵌套函数     | `true`                  | 空（新层）          | 重新收集     |
| 退出外层函数     | 恢复外层值              | 恢复外层列表        | 状态还原     |

---

## 限制与注意事项

### 1. 不支持闭包捕获

**限制**：嵌套函数无法访问外层函数的局部变量

```rust
fn outer(x: i32) -> i32 {
    fn inner() -> i32 {
        x  // ❌ 错误：无法捕获外层变量 x
    }
    inner()
}
```

**原因**：

- 提升为顶层函数后没有保留外层作用域
- 需要闭包转换（closure lowering）才能实现捕获

### 2. 名称冲突风险

**限制**：不同外层函数中的同名嵌套函数会冲突

```rust
fn outer1() {
    fn helper() { }  // 生成 @helper
}

fn outer2() {
    fn helper() { }  // ❌ 生成 @helper，符号冲突
}
```

**解决方案**：

- 需要实现名称修饰（name mangling），如 `@outer1_helper`、`@outer2_helper`
- 当前代码未实现此机制

### 3. 递归嵌套限制

**支持**：多层嵌套递归处理（通过 `outer_nested_functions` 栈）

```rust
fn level1() {
    fn level2() {
        fn level3() {
            // ✅ 支持多层嵌套
        }
        level3();
    }
    level2();
}
```

**生成顺序**：level1 → level2 → level3（深度优先）

---

## 与其他特性的交互

### 1. Local Struct

**协同机制**：

- Local struct 也收集到 `local_structs_set_`（见 `collect_all_structs`）
- 在模块顶部提前生成所有 struct 类型定义
- 嵌套函数内的 local struct 同样会被提升

### 2. Impl 块

**限制**：

- 当前不支持在函数内定义 impl 块
- `ItemStmt::visit` 未处理 `ImplBlock` 类型

### 3. Const 声明

**支持**：

- 函数内 const 声明转换为局部不可变变量（alloca）
- 不生成全局常量（见 `src/ir/ir_generator_statements.cpp:295-324`）

---

## 潜在改进方向

### 1. 名称修饰（Name Mangling）

**目标**：避免嵌套函数名称冲突

**实现思路**：

```cpp
// 在 visit_function_decl 中
std::string mangled_name = current_function_name_ + "_" + node->name.lexeme;
```

### 2. 闭包支持

**目标**：允许嵌套函数捕获外层变量

**实现思路**：

1. 分析捕获变量列表
2. 生成环境结构体（closure environment）
3. 修改函数签名添加隐藏的环境指针参数
4. 转换变量访问为环境指针解引用

### 3. 延迟类型检查

**问题**：当前嵌套函数的语义分析可能在外层函数未完成时执行

**改进**：

- 在 IR 生成前确保所有函数（包括嵌套）的类型已解析
- 或在 IR 生成时延迟解析嵌套函数的符号

---

## 调试与验证

### 测试用例位置

```bash
# 搜索包含嵌套函数的测试用例
grep -r "fn.*{.*fn" testcases/ test_samples/
```

### 生成 IR 验证

```bash
# 编译带嵌套函数的测试文件
./build/code test_samples/nested_fn.rs --emit-ir > output.ll

# 验证 IR 结构
grep "define.*@" output.ll  # 查看生成的所有函数
```

### 常见问题排查

**问题 1**：嵌套函数未生成 IR

- 检查 `inside_function_body_` 是否正确设置
- 确认 `nested_functions_` 是否成功收集

**问题 2**：多层嵌套顺序错误

- 验证 `outer_nested_functions` 的保存/恢复逻辑
- 检查递归调用 `visit_function_decl` 的时机

**问题 3**：变量作用域混淆

- 确保每次进入函数时调用 `value_manager_.enter_scope()`
- 退出时正确调用 `exit_scope()`

---

## 相关文件

| 文件                                 | 职责                           |
| ------------------------------------ | ------------------------------ |
| `src/ir/ir_generator.h`              | 声明嵌套函数相关字段           |
| `src/ir/ir_generator_main.cpp`       | 实现函数生成与嵌套处理流程     |
| `src/ir/ir_generator_statements.cpp` | ItemStmt 中检测并收集嵌套函数  |
| `src/ast/ast.h`                      | 定义 `FnDecl`、`ItemStmt` 结构 |

---

## 总结

当前嵌套函数支持机制通过**延迟提升（Hoisting）**策略实现：

- ✅ 支持多层嵌套函数
- ✅ 正确处理 local struct 和 const 声明
- ❌ 不支持闭包捕获
- ❌ 可能存在名称冲突风险

这是一种轻量级实现，适用于不依赖闭包语义的简单嵌套场景。若需完整闭包支持，需额外实现捕获分析和环境传递机制。
