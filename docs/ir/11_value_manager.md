# 值管理器

ValueManager 负责管理变量作用域、符号查找和值存储。

## 文件位置

`src/ir/value_manager.h`, `src/ir/value_manager.cpp`

## 核心功能

1. **作用域管理**: 进入/退出代码块
2. **变量注册**: 记录变量名到 LLVM 寄存器的映射
3. **符号查找**: 查找变量的 LLVM 值
4. **类型跟踪**: 记录每个变量的类型

## 数据结构

```cpp
class ValueManager {
private:
    struct VariableInfo {
        std::string llvm_name;      // LLVM 寄存器名 (如 "%x.addr")
        const Type *type;            // 变量类型
    };

    std::vector<std::unordered_map<std::string, VariableInfo>> scopes_;
};
```

**scopes\_**: 作用域栈，每个元素是一个符号表

## 作用域管理

### 进入作用域

```cpp
void ValueManager::enter_scope() {
    scopes_.emplace_back();  // 压入新作用域
}
```

**何时调用**:

- 函数开始
- 块表达式开始
- if/while 分支开始

### 退出作用域

```cpp
void ValueManager::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();  // 弹出当前作用域
    }
}
```

**何时调用**:

- 函数结束
- 块表达式结束
- if/while 分支结束

### 示例

```rust
fn main() {
    let x = 1;           // 作用域0
    {
        let y = 2;       // 作用域1
        let x = 3;       // 作用域1 (遮蔽外层x)
    }
    // y 不可见，x 恢复为外层
}
```

**作用域变化**:

```
进入 main()    → 作用域0: {}
let x          → 作用域0: {x → %x.addr}
进入块         → 作用域1: {}
let y          → 作用域1: {y → %y.addr}
let x          → 作用域1: {x → %x.addr.1, y → %y.addr}
退出块         → 作用域0: {x → %x.addr}
退出 main()    → (空)
```

## 变量注册

```cpp
void ValueManager::register_variable(
    const std::string &name,
    const std::string &llvm_name,
    const Type *type
) {
    if (scopes_.empty()) {
        scopes_.emplace_back();
    }

    scopes_.back()[name] = VariableInfo{llvm_name, type};
}
```

**调用示例**:

```cpp
// 生成 let x: i32 = 42;
std::string alloca_name = "%x.addr";
ir_emitter_.emit_alloca(alloca_name, "i32", 4);
value_manager_.register_variable("x", alloca_name, x_type);
```

## 符号查找

```cpp
std::optional<VariableInfo> ValueManager::lookup(
    const std::string &name
) const {
    // 从内到外搜索作用域
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto var_it = it->find(name);
        if (var_it != it->end()) {
            return var_it->second;
        }
    }
    return std::nullopt;  // 未找到
}
```

**查找规则**:

1. 从最内层作用域开始
2. 逐层向外搜索
3. 返回第一个匹配的变量
4. 支持变量遮蔽

### 变量遮蔽示例

```rust
let x = 1;      // 外层 x
{
    let x = 2;  // 内层 x (遮蔽外层)
    print(x);   // 使用内层 x
}
print(x);       // 使用外层 x
```

**IR 生成**:

```llvm
%x.addr = alloca i32           ; 外层 x
store i32 1, i32* %x.addr
; 进入内层作用域
%x.addr.1 = alloca i32         ; 内层 x (不同寄存器)
store i32 2, i32* %x.addr.1
%1 = load i32, i32* %x.addr.1  ; 查找 x → 找到内层
call i32 @print(i32 %1)
; 退出内层作用域
%2 = load i32, i32* %x.addr    ; 查找 x → 找到外层
call i32 @print(i32 %2)
```

## 函数参数处理

函数参数也需要注册：

```rust
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

**IR 生成**:

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
    %a.addr = alloca i32
    store i32 %a, i32* %a.addr    ; 参数复制到栈
    %b.addr = alloca i32
    store i32 %b, i32* %b.addr

    ; 注册: a → %a.addr, b → %b.addr

    %1 = load i32, i32* %a.addr
    %2 = load i32, i32* %b.addr
    %3 = add i32 %1, %2
    ret i32 %3
}
```

**注册代码**:

```cpp
for (auto &param : func_def->parameters) {
    std::string alloca_name = "%" + param.name + ".addr";
    ir_emitter_.emit_alloca(alloca_name, param_type, align);
    value_manager_.register_variable(
        param.name, alloca_name, param.type.get()
    );
}
```

## 结构体字段访问

**不使用 ValueManager**: 字段访问通过 GEP 指令直接计算

```rust
struct Point { x: i32, y: i32 }
let p = Point { x: 1, y: 2 };
let x = p.x;
```

**IR**:

```llvm
%p.addr = alloca %Point          ; 注册 p → %p.addr
; ... 初始化 p ...
%1 = getelementptr %Point, %Point* %p.addr, i32 0, i32 0  ; &p.x
%2 = load i32, i32* %1           ; 加载 p.x
```

**查找顺序**:

1. 查找基址变量 `p` → `%p.addr`
2. 使用 GEP 计算字段地址
3. 加载字段值

## 作用域与控制流

### if 表达式

```rust
let x = if cond {
    let y = 1;
    y + 1
} else {
    let z = 2;
    z + 1
};
```

**作用域管理**:

```cpp
// then 分支
enter_scope();
    // let y ...
    register_variable("y", "%y.addr", ...);
exit_scope();  // y 销毁

// else 分支
enter_scope();
    // let z ...
    register_variable("z", "%z.addr", ...);
exit_scope();  // z 销毁
```

### while 循环

```rust
while i < 10 {
    let temp = i * 2;
    i = i + 1;
}
```

**作用域管理**:

```cpp
// 每次迭代使用同一作用域
enter_scope();
while (cond) {
    // let temp ...
    // temp 在每次迭代中遮蔽前一次
}
exit_scope();
```

**注意**: 循环变量不会跨迭代累积

## 全局变量

**当前不支持**: ValueManager 仅处理局部变量

全局常量（如字符串字面量）由 IRGenerator 直接管理。

## 错误处理

### 变量未声明

```rust
fn main() {
    x = 42;  // 错误: x 未声明
}
```

**检查**:

```cpp
auto var_info = value_manager_.lookup("x");
if (!var_info) {
    error_handler_.report("Undefined variable: x");
}
```

**阶段**: 应由语义分析捕获，IR 生成假设语义正确

### 作用域不匹配

**问题**: 退出作用域次数多于进入次数

**防御**:

```cpp
void ValueManager::exit_scope() {
    if (scopes_.empty()) {
        // 记录警告，不崩溃
        return;
    }
    scopes_.pop_back();
}
```

## 内存管理

**优化**: ValueManager 不负责 alloca 生成，仅记录映射

实际的 alloca 由 IREmitter 统一提升到函数入口：

- ✅ 所有 alloca 在 entry 块
- ✅ 减少栈帧大小
- ✅ 优化寄存器分配

## 测试覆盖

通过集成测试验证：

- ✅ 基础变量查找
- ✅ 变量遮蔽
- ✅ 嵌套作用域
- ✅ 函数参数
- ✅ 控制流作用域（if/while）
- ✅ 结构体和数组作为变量

## 实现细节

### 类型信息存储

```cpp
struct VariableInfo {
    std::string llvm_name;
    const Type *type;  // 原始指针，生命周期由 SemanticAnalyzer 管理
};
```

**注意**: Type 指针必须在语义分析期间保持有效

### 寄存器命名

- 局部变量: `%name.addr`
- 遮蔽变量: `%name.addr.1`, `%name.addr.2`, ...
- 临时值: `%1`, `%2`, `%3`, ...

**保证**: ValueManager 不生成临时寄存器名，仅管理命名变量

---

**参见**:

- [IR 发射器](./09_ir_emitter.md) - Alloca 提升
- [语句生成](./04_statements.md) - let 语句处理
- [控制流](./05_control_flow.md) - 作用域管理
- [函数生成](./02_main_and_functions.md) - 参数注册
