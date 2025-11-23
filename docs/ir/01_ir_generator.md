# IRGenerator 主类设计

## 类定义

`IRGenerator` 是 IR 生成模块的核心控制器，继承自 `ExprVisitor<void>` 和 `StmtVisitor`，使用访问者模式遍历 AST。

## 核心成员

### 1. 依赖组件

```cpp
IREmitter emitter_;              // IR 指令发射器
TypeMapper type_mapper_;         // 类型映射器
ValueManager value_manager_;     // 变量作用域管理器
BuiltinTypes &builtin_types_;    // 内置类型系统
```

### 2. 表达式结果管理

```cpp
std::unordered_map<const void*, std::string> expr_results_;
```

**设计原因**: 访问者模式的 `visit()` 方法返回 `void`，无法直接返回表达式结果。使用 AST 节点指针作为 key 存储结果。

**使用方法**:

```cpp
// 存储结果
store_expr_result(node, "%123");

// 获取结果
std::string result = get_expr_result(node);
```

### 3. 目标地址传递

```cpp
std::string target_address_;
```

**优化目的**: 对于聚合类型（数组、结构体），父表达式可以提供目标地址，子表达式直接在目标位置初始化，避免创建临时变量后拷贝。

**使用流程**:

```cpp
// LetStmt: let arr = [0; 100];
std::string alloca = emit_alloca("[100 x i32]");
set_target_address(alloca);  // 提供目标
node->initializer->accept(this);  // 子表达式原地初始化
take_target_address();  // 清除
```

### 4. 控制流状态

```cpp
bool current_block_terminated_;  // 当前块是否已终止
std::string current_block_label_; // 当前块标签

struct LoopContext {
    std::string continue_label;
    std::string break_label;
};
std::vector<LoopContext> loop_stack_;  // 循环栈
```

**用途**:

- `current_block_terminated_`: 避免在已终止块后生成死代码
- `loop_stack_`: 支持 `break`/`continue` 跳转到正确标签

### 5. 函数状态

```cpp
bool current_function_uses_sret_;      // 当前函数是否使用 SRET
std::string current_function_return_type_str_;  // 当前函数返回类型
bool inside_function_body_;            // 是否在函数体内
```

### 6. 其他状态

```cpp
bool generating_lvalue_;  // 是否在生成左值（赋值目标）
int if_counter_;          // if 表达式计数器
int loop_counter_;        // 循环计数器
std::map<std::string, std::string> const_values_;  // 常量值表
std::map<Type*, size_t> type_size_cache_;  // 类型大小缓存
```

## 核心方法

### 1. 程序生成

```cpp
std::string generate(Program *program)
```

**流程**:

1. 发射类型定义（结构体）
2. 声明内置函数（`printf`, `scanf`, `exit`）
3. 发射常量定义
4. 处理所有顶层 items（函数、impl 方法）
5. 生成 `main` 包装器

**输出结构**:

```llvm
; 类型定义
%Struct1 = type { i32, i32 }

; 常量
@CONST1 = constant i32 100

; 内置函数声明
declare i32 @printf(i8*, ...)

; 用户函数
define i32 @user_func(...) { ... }

; main 包装器
define i32 @main() {
    call void @用户main()
    ret i32 0
}
```

### 2. 表达式访问者

所有 `visit(Expr*)` 方法：

- 生成计算表达式值的 IR
- 将结果（寄存器名或指针）存储到 `expr_results_`
- 不返回任何值

**示例**:

```cpp
void visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);

    std::string left_val = get_expr_result(node->left.get());
    std::string right_val = get_expr_result(node->right.get());

    std::string result = emitter_.emit_add("i32", left_val, right_val);
    store_expr_result(node, result);
}
```

### 3. 语句访问者

所有 `visit(Stmt*)` 方法：

- 生成执行语句的 IR
- 更新控制流状态
- 不存储结果（语句无返回值）

## 设计模式

### 1. 访问者模式

**优点**:

- 清晰的职责分离
- 易于扩展新的 AST 节点类型
- 类型安全的遍历

### 2. 策略模式

不同类型的表达式使用不同的生成策略：

- 基础类型: 直接使用值
- 聚合类型: 使用指针
- 引用类型: 不创建 alloca

### 3. 观察者模式

`ValueManager` 观察作用域变化，自动管理变量生命周期。

## 线程安全

**当前状态**: 非线程安全

**原因**: 使用了大量可变状态（`expr_results_`, `current_block_label_` 等）

**建议**: 每个编译任务使用独立的 `IRGenerator` 实例

## 内存管理

**策略**:

- 不拥有 AST 节点（由 Parser 管理）
- 不拥有 `BuiltinTypes`（由 SemanticAnalyzer 管理）
- 输出字符串由调用方管理

## 错误处理

**当前策略**: 最佳努力生成（Best-effort）

- 遇到无法处理的节点时，生成默认值或跳过
- 依赖语义分析器保证 AST 正确性
- 不抛出异常，避免中断编译流程

**改进方向**: 添加诊断信息收集，报告 IR 生成警告

---

**参见**:

- [IR 发射器](./09_ir_emitter.md) - 底层指令生成
- [类型映射](./10_type_mapper.md) - 类型转换规则
- [值管理](./11_value_manager.md) - 作用域管理
