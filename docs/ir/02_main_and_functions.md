# 函数与程序生成

本模块实现程序入口、函数定义、impl 方法的 IR 生成。

## 文件位置

`src/ir/ir_generator_main.cpp`

## 核心功能

### 1. 程序入口生成 (`generate`)

#### 流程

```cpp
std::string IRGenerator::generate(Program *program)
```

**步骤**:

1. **类型定义发射**: 遍历结构体，生成 LLVM 类型定义
2. **内置函数声明**: 声明 `printf`, `scanf`, `exit` 等
3. **常量定义**: 处理 `const` 声明，生成全局常量
4. **函数处理**: 处理所有函数和 impl 方法
5. **main 包装器**: 生成调用用户 `main` 的包装器

#### 类型定义示例

```rust
struct Point {
    x: i32,
    y: i32,
}
```

生成:

```llvm
%Point = type { i32, i32 }
```

#### 内置函数声明

```llvm
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare void @exit(i32)
@.str.int_format = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.str.int_scanf = private unnamed_addr constant [3 x i8] c"%d\00"
```

#### main 包装器

```llvm
define i32 @main() {
entry:
  call void @用户main()
  ret i32 0
}
```

**目的**: 提供标准的 C `main` 函数入口点，调用用户定义的 `main`。

### 2. 函数定义 (`visit(FnDecl*)`)

#### SRET 优化决策

```cpp
bool should_use_sret_optimization(const std::string &func_name, Type *return_type)
```

**规则**:

- 返回类型是结构体（`TypeKind::STRUCT`）
- 结构体大小 > 0
- 不是特殊函数（如 `main`）

**效果**:

```rust
fn get_point() -> Point { ... }
```

不使用 SRET:

```llvm
define %Point @get_point() { ... }
```

使用 SRET:

```llvm
define void @get_point(%Point* %sret_ptr) {
  ; 直接写入 sret_ptr
  ...
  ret void
}
```

#### 函数签名生成

##### 1. 返回类型处理

```cpp
std::string ret_type_str = type_mapper_.map(return_type_ptr);
bool use_sret = should_use_sret_optimization(func_name, return_type_ptr);
std::string actual_ret_type = use_sret ? "void" : ret_type_str;
```

##### 2. 参数列表构建

**SRET 参数**:

```cpp
if (use_sret) {
    params.push_back({ret_type_str + "*", "sret_ptr"});
}
```

**普通参数**:

- 聚合类型（数组/结构体）: 传指针 `T*`
- 基础类型: 传值 `T`
- 可变引用: 添加 `noalias` 属性

```cpp
// 示例: fn process(arr: [i32; 100], x: &mut i32)
// 生成: define void @process([100 x i32]* %arr, i32* noalias %x)
```

##### 3. 函数头发射

```cpp
emitter_.begin_function(actual_ret_type, func_name, params);
begin_block("bb.entry");
```

#### 参数处理

##### 1. SRET 参数注册

```cpp
if (use_sret) {
    value_manager_.define_variable("__sret_self", "%sret_ptr",
                                    ret_type_str + "*", false);
}
```

**用途**: `self` 引用在方法中映射到 `%sret_ptr`。

##### 2. 普通参数

**引用类型**: 直接使用，不创建 alloca

```cpp
if (is_reference) {
    value_manager_.define_variable(param_name, param_ir_name,
                                    param_type_str, is_mutable);
}
```

**聚合类型**: 拷贝到本地 alloca

```cpp
if (is_aggregate) {
    std::string local_alloca = emitter_.emit_alloca(param_type_str);
    size_t size_bytes = get_type_size(resolved);
    emitter_.emit_memcpy(local_alloca, param_ir_name, size_bytes, ptr_type);
    value_manager_.define_variable(param_name, local_alloca, ...);
}
```

**基础类型**: alloca + store

```cpp
std::string alloca_name = emitter_.emit_alloca(param_type_str);
emitter_.emit_store(param_type_str, param_ir_name, alloca_name);
value_manager_.define_variable(param_name, alloca_name, ...);
```

#### 函数体生成

```cpp
if (node->body.has_value()) {
    body->accept(this);

    // 获取最终表达式结果
    std::string body_result;
    if (body->final_expr.has_value()) {
        body_result = get_expr_result(final_expr.get());
    }

    // 生成返回指令
    if (!current_block_terminated_) {
        if (use_sret) {
            // SRET: 拷贝结果到 sret_ptr
            if (!body_result.empty() && return_type_ptr) {
                size_t size_bytes = get_type_size(return_type_ptr);
                emitter_.emit_memcpy("%sret_ptr", body_result, size_bytes, ptr_type);
            }
            emitter_.emit_ret_void();
        } else if (ret_type_str == "void") {
            emitter_.emit_ret_void();
        } else if (!body_result.empty()) {
            // 聚合类型: load 整个值
            if (ret_is_aggregate) {
                std::string loaded_value = emitter_.emit_load(ret_type_str, body_result);
                emitter_.emit_ret(ret_type_str, loaded_value);
            } else {
                emitter_.emit_ret(ret_type_str, body_result);
            }
        } else {
            emitter_.emit_ret(ret_type_str, "0");
        }
    }
}
```

#### 清理

```cpp
current_block_terminated_ = false;
current_function_uses_sret_ = false;
current_function_return_type_str_ = "";
value_manager_.exit_scope();
```

### 3. impl 方法处理

#### self 参数特殊处理

```rust
impl Foo {
    fn method(&self, x: i32) -> i32 { ... }
}
```

生成:

```llvm
define i32 @Foo_method(%Foo* %self, i32 %x) {
  ; self 已经是指针，直接注册
  ; 不创建额外 alloca
}
```

#### 方法名处理

```cpp
std::string method_name = impl_type_name + "_" + fn_decl->name.lexeme;
// 例如: Foo_method, Point_new
```

### 4. 常量处理

```cpp
void IRGenerator::visit(ConstDecl *node)
```

**策略**:

```rust
const MAX: usize = 100;
```

生成:

```llvm
@MAX = constant i32 100
```

**常量表维护**:

```cpp
const_values_[name] = evaluated_value;
```

**用途**: 在编译时常量表达式求值中使用。

## 关键优化

### 1. SRET 优化

**节省**: 避免返回大结构体时的拷贝

**测试用例**: comprehensive19 中的 `Food_better` 方法

**效果对比**:

```
无 SRET:
  - 调用方: alloca → call → memcpy from 返回值
  - 被调用方: alloca → 构造 → load → ret

有 SRET:
  - 调用方: alloca → call (传指针)
  - 被调用方: 直接写入 sret_ptr → ret void
```

### 2. 参数拷贝优化

**聚合类型参数**: 按指针传递，在被调用方拷贝到本地

**优点**:

- 避免调用方创建临时拷贝
- 被调用方可以选择不拷贝（如果只读）

### 3. Alloca 提升

所有 `alloca` 在 `begin_function` 后立即发射到 `bb.entry` 块。

**好处**:

- 栈帧布局确定
- 避免在循环中重复 alloca
- 支持 LLVM 优化（Mem2Reg）

## 已知问题

### 1. 返回类型不匹配

**问题**: 字面量 `0` 类型为 `i32`，但函数返回 `usize`

**解决**: 添加 `current_function_return_type_str_` 跟踪返回类型，在 `ReturnStmt` 中检查并插入类型转换（已修复）

### 2. SRET 结果未拷贝

**问题**: 使用 SRET 时，函数体结果未拷贝到 `%sret_ptr`

**解决**: 在函数结束时添加 `memcpy` 拷贝（已修复）

## 测试覆盖

- ✅ 基础函数定义
- ✅ 参数传递（值、引用、聚合类型）
- ✅ 返回值（基础类型、结构体）
- ✅ SRET 优化
- ✅ impl 方法
- ✅ 递归函数
- ✅ 多参数函数

---

**参见**:

- [语句生成](./04_statements.md) - 函数体语句处理
- [表达式生成](./03_expressions.md) - 返回值表达式
- [IR 发射器](./09_ir_emitter.md) - begin_function 实现
