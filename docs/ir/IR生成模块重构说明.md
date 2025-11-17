# IR 生成模块重构说明 v2

## 📁 文件结构

重构后的 IR 生成模块由以下文件组成：

### 核心文件

#### 1. `ir_generator_main.cpp` (332 行) ✨ **主入口模块**

**IR 生成器主入口和 Item 处理**

包含：

- 构造函数和主入口 `generate()`
- Item 处理（函数、结构体、const、impl 块）
- 函数代码生成主流程
- visit_function_item() / visit_struct_item() / visit_const_item() / visit_impl_item()

#### 2. `ir_generator_statements.cpp` (200 行) ✨ **语句处理模块**

**Statement visitors 实现**

职责：

- 语句节点访问：Block、Let、Return、Expression Statement、Empty、Break、Continue
- visit_block_stmt() / visit_let_stmt() / visit_return_stmt() 等

#### 3. `ir_generator_expressions.cpp` (586 行) ✨ **基础表达式模块**

**基础表达式 visitors 实现**

职责：

- 基础表达式：Literal、Variable、Binary、Unary、Assign、Call、As、Group
- visit_literal_expr() / visit_binary_expr() / visit_unary_expr() / visit_call_expr() 等
- 处理算术运算、逻辑运算、类型转换

#### 4. `ir_generator_control_flow.cpp` (250 行) ✨ **控制流模块**

**控制流表达式实现**

职责：

- 控制流表达式：If、While、Loop、Block Expression
- visit_if_expr() / visit_while_expr() / visit_loop_expr() / visit_block_expr()
- 基本块管理、phi 节点生成

#### 5. `ir_generator_complex_exprs.cpp` (414 行) ✨ **复杂表达式模块**

**复杂数据结构表达式实现**

职责：

- 数组：ArrayExpr、ArrayRepeatExpr、IndexExpr
- 结构体：StructExpr、FieldAccessExpr
- visit_array_expr() / visit_struct_expr() / visit_index_expr() / visit_field_access_expr()

#### 6. `ir_generator_builtins.cpp` (167 行) ✨ **内置函数模块**

**内置函数支持**

职责：

- 声明 C 标准库函数（printf, scanf, exit）
- 定义格式化字符串常量
- 实现内置 I/O 函数：
  - `printInt(n: i32)` - 输出整数（不换行）
  - `printlnInt(n: i32)` - 输出整数（换行）
  - `getInt() -> i32` - 读取整数
  - `exit(code: i32)` - 程序退出

关键实现：

- 使用 vararg 调用约定（`call i32 (i8*, ...) @printf(...)`）
- exit 后添加 unreachable 指令
- getelementptr 获取格式化字符串指针

#### 7. `ir_generator_helpers.cpp` (118 行) ✨ **辅助工具模块**

**辅助工具函数**

职责：

- 表达式结果管理（get_expr_result, store_expr_result）
- 基本块管理（begin_block）
- Token 到 IR 运算符转换（token_to_ir_op, token_to_icmp_pred）
- 类型判断（is_signed_integer, get_integer_bits）

工具函数：

- `token_to_ir_op()` - 算术运算符转换（+→add, -→sub, \*→mul 等）
- `token_to_icmp_pred()` - 比较运算符转换（==→eq, <→slt 等）
- `is_signed_integer()` - 判断是否为有符号整数（用于选择 sext/zext）
- `get_integer_bits()` - 获取类型位宽（32 位平台：i32/u32/isize/usize 都是 32 位）

### 支持文件

#### 8. `ir_emitter.cpp` / `ir_emitter.h` (373/320 行)

**IR 文本生成器**

提供底层 IR 指令生成接口：

- 指令生成（alloca, load, store, add, icmp 等）
- 函数定义（begin_function, end_function）
- 基本块管理（begin_basic_block）
- 类型定义（emit_struct_type）
- 特殊调用（emit_vararg_call）

#### 9. `type_mapper.cpp` / `type_mapper.h` (228/94 行)

**类型映射器**

将 Rust 类型映射到 LLVM 类型：

- `i32/u32` → `i32`
- `isize/usize` → `i32` (32 位平台)
- `bool` → `i1`
- `[T; N]` → `[N x T]`
- `&T` → `T*`

#### 10. `value_manager.cpp` / `value_manager.h` (111/149 行)

**变量管理器**

管理变量作用域和 alloca 指针：

- 作用域管理（enter_scope, exit_scope）
- 变量声明和查找

#### 11. `ir_generator.h` (221 行)

**头文件**

包含 IRGenerator 类的完整声明：

- 所有 visit 方法声明
- 私有成员变量
- 辅助方法声明
- 可变性检查

## 🔄 重构改进 v2

### 改进点

1. **高度模块化**

   - 将 1917 行的单文件拆分成 **7 个逻辑模块**
   - 每个模块职责清晰、独立、易于维护
   - **最大文件不超过 600 行**

2. **按功能分类**

   - **主入口模块**：函数/结构体/impl 块处理
   - **语句模块**：所有 Statement visitors
   - **基础表达式模块**：字面量、运算、调用、赋值
   - **控制流模块**：if/while/loop 表达式
   - **复杂表达式模块**：数组、结构体、索引、字段访问
   - **内置函数模块**：I/O 函数实现
   - **辅助工具模块**：通用工具函数

3. **32 位平台适配**

   - `usize/isize` 从 64 位改为 32 位
   - 适配 32 位运行环境

4. **代码可读性**

   - 每个文件都有详细的文档注释
   - 说明模块职责和关键实现
   - 文件头注释清晰标明职责

5. **调试友好**
   - 按功能拆分，问题定位更快
   - 文件小，编译增量更新快
   - 修改某类功能只需关注对应模块

### 文件大小对比

| 文件                                | 行数        | 说明                         |
| ----------------------------------- | ----------- | ---------------------------- |
| **重构前**                          |             |                              |
| ir_generator.cpp (v1)               | 1917 行     | 单一大文件                   |
| **重构中间版 (v1.5)**               |             |                              |
| ir_generator.cpp                    | 1692 行     | 核心逻辑                     |
| ir_generator_builtins.cpp           | 167 行      | 内置函数                     |
| ir_generator_helpers.cpp            | 118 行      | 辅助函数                     |
| 小计                                | 1977 行     | 3 个文件                     |
| **重构后 (v2 - 当前版本)**          |             |                              |
| ir_generator_main.cpp               | **332 行**  | 主入口和 Item 处理           |
| ir_generator_statements.cpp         | **200 行**  | Statement visitors           |
| ir_generator_expressions.cpp        | **586 行**  | 基础表达式 visitors          |
| ir_generator_control_flow.cpp       | **250 行**  | 控制流表达式                 |
| ir_generator_complex_exprs.cpp      | **414 行**  | 复杂数据结构表达式           |
| ir_generator_builtins.cpp           | **167 行**  | 内置函数                     |
| ir_generator_helpers.cpp            | **118 行**  | 辅助工具                     |
| **小计**                            | **2067 行** | **7 个文件，最大不超过 600** |
| **支持文件**                        |             |                              |
| ir_emitter.cpp / ir_emitter.h       | 373 + 320   | IR 文本生成器                |
| type_mapper.cpp / type_mapper.h     | 228 + 94    | 类型映射                     |
| value_manager.cpp / value_manager.h | 111 + 149   | 变量管理                     |
| ir_generator.h                      | 221         | 头文件                       |
| **总计**                            | **3563 行** | 完整 IR 生成模块             |

## 🎯 使用指南

### 添加新的内置函数

在 `ir_generator_builtins.cpp` 中：

1. 在 `emit_builtin_declarations()` 添加函数声明
2. 在 `handle_builtin_function()` 添加处理逻辑

示例：添加 printBool 函数

```cpp
// 在emit_builtin_declarations()中：
emitter_.emit_global_variable(".str.bool_true", "[5 x i8]", "c\"true\\00\"", true);
emitter_.emit_global_variable(".str.bool_false", "[6 x i8]", "c\"false\\00\"", true);

// 在handle_builtin_function()中：
if (func_name == "printBool") {
    // 处理逻辑...
}
```

### 添加新的语句类型

在 `ir_generator_statements.cpp` 中添加新的 `visit_xxx_stmt()` 方法。

### 添加新的表达式类型

- **基础表达式**：在 `ir_generator_expressions.cpp` 中添加
- **控制流表达式**：在 `ir_generator_control_flow.cpp` 中添加
- **复杂数据结构**：在 `ir_generator_complex_exprs.cpp` 中添加

### 添加新的辅助函数

在 `ir_generator_helpers.cpp` 中添加，并在 `ir_generator.h` 中声明。

### 修改类型映射

在 `type_mapper.cpp` 的 `map_primitive()` 函数中修改。

## 📊 模块依赖关系

```
ir_generator.h (头文件)
    ↓
┌───────────────────────────────────────────────┐
│                                               │
├─ ir_generator_main.cpp (主入口)              │
│   ├─ 调用 emit_builtin_declarations()        │
│   ├─ 调用各种 visit_xxx 方法                 │
│   └─ 使用 helpers 中的工具函数               │
│                                               │
├─ ir_generator_statements.cpp (语句处理)      │
│   ├─ 调用表达式 visitors                     │
│   └─ 使用 helpers                            │
│                                               │
├─ ir_generator_expressions.cpp (基础表达式)   │
│   ├─ 调用 handle_builtin_function()          │
│   └─ 使用 helpers (token_to_ir_op 等)        │
│                                               │
├─ ir_generator_control_flow.cpp (控制流)      │
│   ├─ 调用表达式 visitors                     │
│   └─ 使用 helpers (begin_block 等)           │
│                                               │
├─ ir_generator_complex_exprs.cpp (复杂表达式) │
│   ├─ 调用基础表达式 visitors                 │
│   └─ 使用 helpers                            │
│                                               │
├─ ir_generator_builtins.cpp (内置函数)        │
│   └─ 独立模块，被 main 和 expressions 调用   │
│                                               │
└─ ir_generator_helpers.cpp (辅助工具)         │
    └─ 被所有其他模块调用                      │
                                                │
┌───────────────────────────────────────────────┘
│
├─ ir_emitter (底层 IR 生成)
├─ type_mapper (类型映射)
└─ value_manager (变量管理)
```

```
ir_generator.cpp
├── ir_generator_builtins.cpp (内置函数)
├── ir_generator_helpers.cpp (辅助函数)
├── ir_emitter.cpp (IR指令生成)
├── type_mapper.cpp (类型映射)
└── value_manager.cpp (变量管理)
```

## ✅ 测试验证

重构后所有测试通过：

- ✅ test_exit_simple.rs - exit 函数
- ✅ test_as_expr.rs - 类型转换
- ✅ test_const.rs - const 常量
- ✅ test_comprehensive.rs - 综合测试

## 📌 注意事项

1. **32 位平台**：当前配置为 32 位（usize=i32）
2. **编译顺序**：CMakeLists.txt 已更新，包含所有新文件
3. **代码风格**：保持与原有代码一致的命名和注释风格

---

_重构日期：2024-11-13_  
_编译器版本：Simple Rust Compiler Phase 2H+_
