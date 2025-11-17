# ValueManager 实现方案分析

## 核心问题：文档与设计原则冲突

### 🔴 问题发现

文档中的 ValueManager 设计使用了 LLVM API：

```cpp
struct VariableInfo {
    llvm::Value* alloca_inst;   // ❌ 使用了LLVM类型！
    llvm::Type* type;           // ❌ 使用了LLVM类型！
    bool is_mutable;
};
```

**这与我们的核心原则冲突**：

- "不使用 LLVM C++ API"
- "手动生成 LLVM IR 文本"
- "所有 IR 指令都是字符串拼接"

## 正确的设计方案

### 方案：纯字符串实现

既然我们采用手动生成 IR 文本的方式，ValueManager 也应该使用字符串表示：

```cpp
struct VariableInfo {
    std::string alloca_name;     // IR中的变量名，如 "%x.addr"
    std::string type_str;        // IR类型字符串，如 "i32"
    bool is_mutable;             // 是否可变
};
```

### 设计细节

#### 1. 变量信息存储

**VariableInfo 结构**：

```cpp
struct VariableInfo {
    std::string alloca_name;     // alloca指令返回的IR变量名
    std::string type_str;        // 该变量的IR类型字符串
    bool is_mutable;             // 是否可变（Rust语义）

    // 可选：添加更多元数据
    // int line;                 // 定义位置（用于错误报告）
    // std::string source_name;  // 源代码中的变量名
};
```

**示例**：

```rust
let x: i32 = 42;
```

对应的 VariableInfo：

```cpp
VariableInfo {
    alloca_name = "%x.addr",    // IREmitter分配的临时变量名
    type_str = "i32",           // TypeMapper转换的类型
    is_mutable = false
}
```

#### 2. 作用域管理

**Scope 结构**：

```cpp
struct Scope {
    std::unordered_map<std::string, VariableInfo> variables;

    // 可选：添加作用域元数据
    // ScopeType type;  // Function, Block, Loop等
    // std::string label_prefix;  // 用于生成唯一标签
};
```

**作用域栈**：

```cpp
std::vector<Scope> scope_stack_;
```

- 全局作用域在最底层（索引 0）
- 当前作用域在栈顶
- enter_scope()压栈，exit_scope()弹栈

#### 3. 变量定义流程

当遇到`let x: i32 = 42;`时：

```cpp
// 1. IREmitter生成alloca指令
std::string alloca_var = emitter.emit_alloca("i32", "x");
// 返回: "%0"，IR中输出: %0 = alloca i32 ; x

// 2. TypeMapper获取类型字符串
std::string type_str = type_mapper.map(var_type);
// 返回: "i32"

// 3. ValueManager记录变量
value_manager.define_variable("x", alloca_var, type_str, false);
// 存储: { alloca_name="%0", type_str="i32", is_mutable=false }
```

#### 4. 变量使用流程

当遇到`x + 1`时：

```cpp
// 1. ValueManager查找变量
VariableInfo* var_info = value_manager.lookup_variable("x");
// 返回: { alloca_name="%0", type_str="i32", is_mutable=false }

// 2. IREmitter生成load指令
std::string loaded_value = emitter.emit_load(
    var_info->type_str,      // "i32"
    var_info->alloca_name    // "%0"
);
// 返回: "%1"，IR中输出: %1 = load i32, i32* %0

// 3. 使用loaded_value进行后续运算
std::string result = emitter.emit_binary_op("add", "i32", loaded_value, "1");
// 返回: "%2"，IR中输出: %2 = add i32 %1, 1
```

## 详细接口设计

### value_manager.h

```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

/**
 * VariableInfo - 变量信息
 * 存储变量的IR表示和元数据
 */
struct VariableInfo {
    std::string alloca_name;     // IR中alloca返回的变量名（如 "%x.addr"）
    std::string type_str;        // IR类型字符串（如 "i32", "i32*"）
    bool is_mutable;             // 是否可变

    VariableInfo() = default;
    VariableInfo(const std::string& alloca,
                const std::string& type,
                bool mut)
        : alloca_name(alloca), type_str(type), is_mutable(mut) {}
};

/**
 * ValueManager - 变量和值管理器
 *
 * 核心职责:
 * 1. 管理变量作用域栈
 * 2. 变量名到IR变量的映射
 * 3. 支持变量遮蔽(shadowing)
 * 4. 检测重复定义
 */
class ValueManager {
public:
    ValueManager();

    // ========== 作用域管理 ==========

    /**
     * 进入新作用域
     * 例如：函数体、代码块、循环体
     */
    void enter_scope();

    /**
     * 退出当前作用域
     * 注意：不能退出全局作用域
     */
    void exit_scope();

    /**
     * 获取当前作用域深度
     * @return 0表示全局作用域，1表示第一层嵌套，以此类推
     */
    size_t scope_depth() const;

    // ========== 变量操作 ==========

    /**
     * 在当前作用域定义变量
     * @param name 源代码中的变量名
     * @param alloca_name IR中的alloca变量名（如 "%0"）
     * @param type_str IR类型字符串（如 "i32"）
     * @param is_mutable 是否可变
     */
    void define_variable(const std::string& name,
                        const std::string& alloca_name,
                        const std::string& type_str,
                        bool is_mutable);

    /**
     * 查找变量（从当前作用域向外层查找）
     * @param name 源代码中的变量名
     * @return 变量信息指针，未找到返回nullptr
     */
    VariableInfo* lookup_variable(const std::string& name);

    /**
     * 查找变量（const版本）
     */
    const VariableInfo* lookup_variable(const std::string& name) const;

    /**
     * 检查当前作用域是否已定义该变量
     * 用于检测重复定义（不包括shadowing）
     * @param name 源代码中的变量名
     * @return true表示当前作用域已定义
     */
    bool is_defined_in_current_scope(const std::string& name) const;

    /**
     * 检查变量是否存在（在任何作用域）
     * @param name 源代码中的变量名
     * @return true表示变量存在
     */
    bool variable_exists(const std::string& name) const;

    // ========== 调试和辅助 ==========

    /**
     * 获取当前作用域的所有变量名（用于调试）
     */
    std::vector<std::string> get_current_scope_variables() const;

    /**
     * 清空所有作用域（用于测试）
     */
    void clear();

private:
    /**
     * Scope - 单个作用域
     */
    struct Scope {
        std::unordered_map<std::string, VariableInfo> variables;
    };

    std::vector<Scope> scope_stack_;  // 作用域栈
};
```

### value_manager.cpp

```cpp
#include "value_manager.h"

ValueManager::ValueManager() {
    // 创建全局作用域
    enter_scope();
}

void ValueManager::enter_scope() {
    scope_stack_.emplace_back();
}

void ValueManager::exit_scope() {
    // 保留全局作用域，至少保持一个作用域
    if (scope_stack_.size() > 1) {
        scope_stack_.pop_back();
    }
}

size_t ValueManager::scope_depth() const {
    // 全局作用域深度为0
    return scope_stack_.size() - 1;
}

void ValueManager::define_variable(const std::string& name,
                                   const std::string& alloca_name,
                                   const std::string& type_str,
                                   bool is_mutable) {
    if (scope_stack_.empty()) {
        // 不应该发生，构造函数已经创建了全局作用域
        return;
    }

    VariableInfo info(alloca_name, type_str, is_mutable);
    scope_stack_.back().variables[name] = info;
}

VariableInfo* ValueManager::lookup_variable(const std::string& name) {
    // 从最内层向外层查找
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }
    return nullptr;  // 未找到
}

const VariableInfo* ValueManager::lookup_variable(const std::string& name) const {
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }
    return nullptr;
}

bool ValueManager::is_defined_in_current_scope(const std::string& name) const {
    if (scope_stack_.empty()) {
        return false;
    }

    return scope_stack_.back().variables.find(name) !=
           scope_stack_.back().variables.end();
}

bool ValueManager::variable_exists(const std::string& name) const {
    return lookup_variable(name) != nullptr;
}

std::vector<std::string> ValueManager::get_current_scope_variables() const {
    std::vector<std::string> result;
    if (!scope_stack_.empty()) {
        for (const auto& [name, _] : scope_stack_.back().variables) {
            result.push_back(name);
        }
    }
    return result;
}

void ValueManager::clear() {
    scope_stack_.clear();
    enter_scope();  // 重新创建全局作用域
}
```

## 使用示例

### 示例 1：简单变量定义和使用

```rust
fn main() {
    let x: i32 = 42;
    let y: i32 = x + 1;
}
```

**IR 生成过程**：

```cpp
// 函数开始
emitter.begin_function("void", "main", {});
emitter.begin_basic_block("entry");
value_manager.enter_scope();  // 函数作用域

// let x: i32 = 42;
std::string x_alloca = emitter.emit_alloca("i32", "x");  // %0 = alloca i32
emitter.emit_store("i32", "42", x_alloca);               // store i32 42, i32* %0
value_manager.define_variable("x", x_alloca, "i32", false);

// let y: i32 = x + 1;
VariableInfo* x_info = value_manager.lookup_variable("x");
std::string x_val = emitter.emit_load("i32", x_info->alloca_name);  // %1 = load i32, i32* %0
std::string y_val = emitter.emit_binary_op("add", "i32", x_val, "1"); // %2 = add i32 %1, 1

std::string y_alloca = emitter.emit_alloca("i32", "y");  // %3 = alloca i32
emitter.emit_store("i32", y_val, y_alloca);              // store i32 %2, i32* %3
value_manager.define_variable("y", y_alloca, "i32", false);

emitter.emit_ret_void();
value_manager.exit_scope();
emitter.end_function();
```

**生成的 IR**：

```llvm
define void @main() {
entry:
  %0 = alloca i32 ; x
  store i32 42, i32* %0
  %1 = load i32, i32* %0
  %2 = add i32 %1, 1
  %3 = alloca i32 ; y
  store i32 %2, i32* %3
  ret void
}
```

### 示例 2：变量遮蔽(Shadowing)

```rust
fn main() {
    let x: i32 = 10;
    {
        let x: i32 = 20;  // 遮蔽外层的x
        // 这里x是20
    }
    // 这里x是10
}
```

**作用域管理**：

```cpp
// 外层作用域
value_manager.enter_scope();  // 深度1
value_manager.define_variable("x", "%0", "i32", false);  // x -> %0

// 内层作用域
value_manager.enter_scope();  // 深度2
value_manager.define_variable("x", "%2", "i32", false);  // x -> %2 (遮蔽外层)

// 在内层查找x
VariableInfo* inner_x = value_manager.lookup_variable("x");
// 返回: { alloca_name="%2", ... }

value_manager.exit_scope();  // 回到深度1

// 在外层查找x
VariableInfo* outer_x = value_manager.lookup_variable("x");
// 返回: { alloca_name="%0", ... }
```

### 示例 3：可变性检查

```rust
let x: i32 = 10;
x = 20;  // 错误：x不可变

let mut y: i32 = 10;
y = 20;  // 正确：y可变
```

**可变性验证**：

```cpp
// let x: i32 = 10;
value_manager.define_variable("x", "%0", "i32", false);

// x = 20;
VariableInfo* x_info = value_manager.lookup_variable("x");
if (!x_info->is_mutable) {
    error_reporter.report_error("Cannot assign to immutable variable 'x'");
}

// let mut y: i32 = 10;
value_manager.define_variable("y", "%1", "i32", true);

// y = 20;
VariableInfo* y_info = value_manager.lookup_variable("y");
if (!y_info->is_mutable) {
    // 不会触发，y是可变的
} else {
    // 允许赋值
    emitter.emit_store("i32", "20", y_info->alloca_name);
}
```

## 测试策略

### 测试用例设计

```cpp
// Test 1: 基础变量定义和查找
void test_basic_define_lookup();

// Test 2: 作用域管理
void test_scope_management();

// Test 3: 变量遮蔽
void test_variable_shadowing();

// Test 4: 重复定义检测
void test_duplicate_definition();

// Test 5: 变量不存在
void test_variable_not_found();

// Test 6: 可变性标记
void test_mutability();

// Test 7: 深层嵌套作用域
void test_nested_scopes();

// Test 8: 全局作用域保护
void test_global_scope_protection();
```

## 潜在问题和解决方案

### 问题 1：循环中的变量

**场景**：

```rust
for i in 0..10 {
    let x: i32 = i;
}
```

**问题**：每次循环迭代都需要新的 alloca 吗？

**解决方案**：

- 在循环 entry 处创建 alloca（只分配一次）
- 每次迭代只做 store
- ValueManager 记录的是循环外的 alloca

### 问题 2：函数参数

**场景**：

```rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

**问题**：函数参数不是 alloca，而是函数参数

**解决方案**：

- 参数在 IR 中是`%a`, `%b`（不是 alloca 结果）
- 需要在函数 entry 创建 alloca，然后 store 参数值
- 或者扩展 VariableInfo，添加`is_parameter`标志

### 问题 3：全局变量

**场景**：

```rust
static X: i32 = 42;
```

**问题**：全局变量在 IR 中是`@X`，不是`%`开头

**解决方案**：

- 扩展 VariableInfo 添加`is_global`标志
- 或者使用不同的前缀（@vs%）来区分

## 建议

### 当前阶段（第一阶段）

1. **实现基础功能**：

   - 作用域管理（enter/exit）
   - 变量定义（define_variable）
   - 变量查找（lookup_variable）
   - 重复定义检测（is_defined_in_current_scope）

2. **暂时简化**：

   - 只处理局部变量（函数内）
   - 函数参数暂时不处理
   - 全局变量留到后续阶段

3. **完整测试**：
   - 所有 8 个测试用例
   - 验证 shadowing 正确性
   - 验证可变性标记

### 后续阶段

1. **扩展功能**：

   - 函数参数处理
   - 全局变量支持
   - 静态变量支持

2. **优化**：
   - 添加变量使用计数（用于优化）
   - 添加生命周期信息
   - 添加调试信息

## 与其他模块的协作

### IREmitter

- ValueManager 使用 IREmitter 返回的临时变量名
- 示例：`std::string alloca_name = emitter.emit_alloca(...)`

### TypeMapper

- ValueManager 使用 TypeMapper 转换的类型字符串
- 示例：`std::string type_str = type_mapper.map(var_type)`

### IRGenerator

- IRGenerator 协调三个模块的使用
- 示例：先 TypeMapper 获取类型，再 IREmitter 生成 alloca，最后 ValueManager 记录

## 总结

✅ **核心修正**：使用纯字符串实现，不依赖 LLVM API

✅ **设计清晰**：三个字符串字段足以表示变量

✅ **功能完整**：支持作用域、shadowing、可变性

✅ **易于测试**：纯数据结构，无副作用

✅ **扩展性好**：可以轻松添加新字段和功能

**建议开始实现**，有任何问题随时讨论！
