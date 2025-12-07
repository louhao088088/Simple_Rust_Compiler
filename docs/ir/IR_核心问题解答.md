# IR 生成核心问题解答

本文档整理了 Simple Rust Compiler 的 IR 生成模块中的核心实现问题及详细解答。

---

## 一、类型系统与内存布局

### 1.1 聚合类型的值传递

**问题**: 为什么数组和结构体参数要按指针传递？

**答案**:

```rust
fn process(arr: [i32; 100]) { ... }
```

生成的 IR 是 **按指针传递**：

```llvm
define void @process([100 x i32]* %arr) {
```

**原因**:

1. **效率**: 避免拷贝大量数据到栈上
2. **ABI 兼容**: 符合 C 调用约定

**修改隔离处理** (`ir_generator_main.cpp` 第 195-202 行):

```cpp
if (is_aggregate) {
    // 在被调用方拷贝到本地 alloca
    std::string local_alloca = emitter_.emit_alloca(param_type_str);
    size_t size_bytes = get_type_size(resolved);
    emitter_.emit_memcpy(local_alloca, param_ir_name, size_bytes, ptr_type);
    value_manager_.define_variable(param_name, local_alloca, ...);
}
```

**效果**: 调用者的数组**不会被修改**，因为被调用方操作的是本地拷贝。

---

### 1.2 结构体字段对齐与大小计算

**问题**: 如何计算结构体大小和对齐？

```rust
struct Mixed {
    a: bool,   // 1 byte, align 1
    b: i32,    // 4 bytes, align 4
}
```

**答案** (`ir_generator_helpers.cpp` 第 488-512 行):

```cpp
size_t IRGenerator::get_type_size(Type *type) {
    // 结构体处理
    if (auto struct_type = dynamic_cast<StructType *>(type)) {
        size_t offset = 0;
        size_t max_align = 1;

        for (const auto &field_name : struct_type->field_order) {
            Type *field_type = it->second.get();
            size_t field_size = get_type_size(field_type);
            size_t field_align = get_type_alignment(field_type);

            if (field_align > max_align) {
                max_align = field_align;
            }

            // 插入填充以满足对齐要求
            if (offset % field_align != 0) {
                offset += (field_align - (offset % field_align));
            }

            offset += field_size;
        }

        // 尾部填充以满足结构体对齐
        if (offset % max_align != 0) {
            offset += (max_align - (offset % max_align));
        }

        return offset;
    }
}
```

**`Mixed` 的布局**:

```
偏移 0: a (bool, 1 byte)
偏移 1-3: padding (3 bytes, 对齐到 4)
偏移 4-7: b (i32, 4 bytes)
总大小: 8 bytes (不是 5 bytes)
```

---

## 二、SRET 优化

### 2.1 SRET 调用约定

**问题**: 返回结构体时调用方和被调用方各做什么？

```rust
let p = get_point();  // fn get_point() -> Point
```

**答案**:

#### 被调用方 (get_point)

```llvm
define void @get_point(%Point* %sret_ptr) {
entry:
  ; 直接在 sret_ptr 上构造结构体
  %x_ptr = getelementptr %Point, %Point* %sret_ptr, i32 0, i32 0
  store i32 1, i32* %x_ptr
  %y_ptr = getelementptr %Point, %Point* %sret_ptr, i32 0, i32 1
  store i32 2, i32* %y_ptr
  ret void
}
```

#### 调用方

```llvm
  %p = alloca %Point              ; 调用方分配空间
  call void @get_point(%Point* %p) ; 传递指针
  ; 现在 %p 已经包含结果，无需额外拷贝
```

**实现代码** (`ir_generator_main.cpp` 第 120-125 行):

```cpp
if (return_type_ptr && should_use_sret_optimization(func_name, return_type_ptr)) {
    use_sret = true;
    params.push_back({ret_type_str + "*", "sret_ptr"});  // 添加隐藏参数
}
```

---

### 2.2 SRET 判断条件

**问题**: 什么情况下使用 SRET？

```cpp
bool IRGenerator::should_use_sret_optimization(const std::string &func_name,
                                                Type *return_type) {
    // 条件1: 返回类型是结构体
    if (return_type->kind != TypeKind::STRUCT) {
        return false;
    }

    // 条件2: 不是 main 函数
    if (func_name == "main") {
        return false;
    }

    // 条件3: 结构体大小 > 0
    size_t size = get_type_size(return_type);
    return size > 0;
}
```

---

## 三、左值与右值

### 3.1 generating*lvalue* 标志

**问题**: 如何区分左值和右值上下文？

```rust
arr[i] = 5;      // arr[i] 是左值 (需要地址)
let x = arr[i];  // arr[i] 是右值 (需要值)
```

**实现** (`ir_generator_complex_exprs.cpp` 第 290-297 行):

```cpp
void IRGenerator::visit(IndexExpr *node) {
    // ... 计算 elem_ptr (GEP) ...

    if (generating_lvalue_ || elem_is_aggregate) {
        // 左值上下文: 返回指针，供 store 使用
        store_expr_result(node, elem_ptr);
    } else {
        // 右值上下文: load 值并返回
        std::string elem_value = emitter_.emit_load(elem_ir_type, elem_ptr);
        store_expr_result(node, elem_value);
    }
}
```

**生成的 IR 对比**:

```llvm
; arr[i] = 5 (左值)
%elem_ptr = getelementptr [10 x i32], [10 x i32]* %arr, i64 0, i64 %i
store i32 5, i32* %elem_ptr

; let x = arr[i] (右值)
%elem_ptr = getelementptr [10 x i32], [10 x i32]* %arr, i64 0, i64 %i
%x = load i32, i32* %elem_ptr
```

---

### 3.2 复合赋值的实现

**问题**: `arr[i] += 5` 如何实现？

**答案**: 只计算一次地址 (`ir_generator_expressions.cpp` 第 997-1050 行):

```cpp
void IRGenerator::visit(CompoundAssignmentExpr *node) {
    // 1. 获取左值地址 (只计算一次 GEP)
    generating_lvalue_ = true;
    node->target->accept(this);
    generating_lvalue_ = false;
    std::string ptr_value = get_expr_result(node->target.get());

    // 2. 从地址加载当前值
    std::string current_value = emitter_.emit_load(type_str, ptr_value);

    // 3. 计算右值
    node->value->accept(this);
    std::string rhs_value = get_expr_result(node->value.get());

    // 4. 执行运算
    std::string result = emitter_.emit_binary_op(op, type_str, current_value, rhs_value);

    // 5. 存回同一地址
    emitter_.emit_store(type_str, result, ptr_value);
}
```

**生成的 IR**:

```llvm
; arr[i] += 5
%ptr = getelementptr [10 x i32], [10 x i32]* %arr, i64 0, i64 %i  ; 只算一次
%old = load i32, i32* %ptr
%new = add i32 %old, 5
store i32 %new, i32* %ptr
```

---

## 四、控制流

### 4.1 短路求值实现

**问题**: `a && b` 的 IR 结构是什么？

**答案** (`ir_generator_expressions.cpp` 第 345-425 行):

```cpp
void IRGenerator::visit_logical_binary_expr(BinaryExpr *node) {
    std::string rhs_label = "and.rhs." + std::to_string(and_counter_++);
    std::string end_label = "and.end." + std::to_string(and_counter_);

    // 计算左操作数
    node->left->accept(this);
    std::string left_var = get_expr_result(node->left.get());
    std::string left_block = current_block_label_;

    // 短路判断
    if (is_or) {
        emitter_.emit_cond_br(left_var, end_label, rhs_label);  // true则跳过右边
    } else {
        emitter_.emit_cond_br(left_var, rhs_label, end_label);  // false则跳过右边
    }

    // 计算右操作数
    begin_block(rhs_label);
    node->right->accept(this);
    std::string right_var = get_expr_result(node->right.get());
    std::string right_block = current_block_label_;
    emitter_.emit_br(end_label);

    // 合并结果 (phi 节点)
    begin_block(end_label);
    std::vector<std::pair<std::string, std::string>> phi_incoming;
    phi_incoming.push_back({left_var, left_block});
    phi_incoming.push_back({right_var, right_block});
    std::string result = emitter_.emit_phi("i1", phi_incoming);
}
```

**生成的 IR** (对于 `a && b`):

```llvm
entry:
  %a = load i1, i1* %a.addr
  br i1 %a, label %and.rhs.1, label %and.end.1  ; a为false直接跳到end

and.rhs.1:
  %b = load i1, i1* %b.addr
  br label %and.end.1

and.end.1:
  %result = phi i1 [ %a, %entry ], [ %b, %and.rhs.1 ]
```

**关键点**: 如果 `a` 为 `false`，`b` **不会被求值**。

---

### 4.2 循环控制流

**问题**: `while` 和 `loop` 的 IR 结构？

#### while 循环

```llvm
  br label %while.cond.0

while.cond.0:
  %cond = <evaluate condition>
  br i1 %cond, label %while.body.0, label %while.end.0

while.body.0:
  <loop body>
  br label %while.cond.0       ; continue 跳这里

while.end.0:                   ; break 跳这里
  ; 继续执行
```

#### loop 循环

```llvm
  br label %loop.body.0

loop.body.0:
  <loop body>
  br label %loop.body.0        ; 无条件跳回

loop.end.0:                    ; 只能通过 break 到达
  ; 继续执行
```

---

## 五、Shadowing 实现

### 5.1 作用域栈机制

**问题**: Variable shadowing 如何实现？

```rust
let x = 5;
let x = x + 1;  // shadow
```

**答案**:

#### 语义分析 (`semantic.cpp`):

```cpp
bool SymbolTable::define_variable(const std::string &name,
                                  std::shared_ptr<Symbol> symbol,
                                  bool allow_shadow) {
    auto &scope = scopes_.back();
    if (scope.value_symbols.find(name) != scope.value_symbols.end()) {
        if (!allow_shadow) return false;
    }
    scope.value_symbols[name] = symbol;  // 直接覆盖
    return true;
}
```

#### IR 生成 (`value_manager.cpp`):

```cpp
void ValueManager::define_variable(const std::string &name,
                                   const std::string &alloca_name, ...) {
    // 在当前作用域定义，覆盖同名变量
    scope_stack_.back().variables[name] = info;
}

VariableInfo* ValueManager::lookup_variable(const std::string &name) {
    // 从内到外查找，返回最近的定义
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        if (found) return &var_it->second;
    }
    return nullptr;
}
```

**生成的 IR**:

```llvm
  %x.addr.1 = alloca i32
  store i32 5, i32* %x.addr.1

  %1 = load i32, i32* %x.addr.1
  %2 = add i32 %1, 1
  %x.addr.2 = alloca i32           ; 新的 alloca (shadow)
  store i32 %2, i32* %x.addr.2
```

**关键**: 每次 `let` 都创建**新的 alloca**，映射表更新为新地址。

---

## 六、函数调用

### 6.1 参数传递策略

| 类型                     | 传递方式              | IR 示例                            |
| ------------------------ | --------------------- | ---------------------------------- |
| 基础类型 (i32, bool)     | 按值传递              | `define void @f(i32 %x)`           |
| 引用类型 (&T)            | 按指针传递            | `define void @f(i32* %x)`          |
| 聚合类型 ([T;N], Struct) | 按指针传递 + 本地拷贝 | `define void @f([10 x i32]* %arr)` |
| 可变引用 (&mut T)        | 指针 + noalias        | `define void @f(i32* noalias %x)`  |

### 6.2 方法调用 (self 参数)

```rust
impl Point {
    fn move_by(&mut self, dx: i32) { self.x += dx; }
}
p.move_by(5);
```

**生成的 IR**:

```llvm
; 方法定义
define void @Point_move_by(%Point* %self, i32 %dx) {
  %x_ptr = getelementptr %Point, %Point* %self, i32 0, i32 0
  %x = load i32, i32* %x_ptr
  %new_x = add i32 %x, %dx
  store i32 %new_x, i32* %x_ptr
  ret void
}

; 调用
call void @Point_move_by(%Point* %p.addr, i32 5)
```

**关键**: `self` 直接是指针，无需额外 alloca。

---

## 七、边界情况

### 7.1 零初始化优化

**问题**: `[0; 1000]` 如何优化？

```cpp
bool IRGenerator::is_zero_initializer(Expr *expr) {
    if (auto lit = dynamic_cast<LiteralExpr *>(expr)) {
        if (lit->literal.type == TokenType::NUMBER) {
            return std::stoll(lit->literal.lexeme) == 0;
        }
    }
    // 递归检查数组和结构体
    ...
}
```

**优化后的 IR**:

```llvm
; 不是 1000 次 store，而是一次 memset
%arr = alloca [1000 x i32]
%arr_ptr = bitcast [1000 x i32]* %arr to i8*
call void @llvm.memset.p0i8.i64(i8* %arr_ptr, i8 0, i64 4000, i1 false)
```

---

### 7.2 嵌套数组索引

**问题**: `arr[1][2]` 的 GEP 如何计算？

```rust
let arr: [[i32; 3]; 2] = ...;
arr[1][2] = 5;
```

**答案**: 需要**两次 GEP**:

```llvm
; 第一次: 获取 arr[1] (类型 [3 x i32])
%row_ptr = getelementptr [[3 x i32]; 2], [[3 x i32]; 2]* %arr, i64 0, i64 1

; 第二次: 获取 arr[1][2] (类型 i32)
%elem_ptr = getelementptr [3 x i32], [3 x i32]* %row_ptr, i64 0, i64 2

store i32 5, i32* %elem_ptr
```

---

### 7.3 递归结构体

**问题**: 链表节点如何表示？

```rust
struct Node {
    value: i32,
    next: &Node,  // 引用避免无限大小
}
```

**生成的 IR**:

```llvm
%Node = type { i32, %Node* }  ; next 是指针，避免无限递归
```

**处理方式**:

1. `collect_all_structs()` 先收集所有结构体声明
2. 按顺序生成类型定义
3. 引用类型映射为指针，避免无限大小

---

## 八、格式字符串常量

```llvm
@.str.int_format = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.str.int_scanf = private unnamed_addr constant [3 x i8] c"%d\00"
```

| 部分               | 含义                         |
| ------------------ | ---------------------------- |
| `@.str.int_format` | 全局变量名                   |
| `private`          | 仅模块内可见                 |
| `unnamed_addr`     | 地址不重要，可合并           |
| `constant`         | 只读常量                     |
| `[4 x i8]`         | 4 字节数组                   |
| `c"%d\0A\00"`      | C 字符串: `%d` + `\n` + `\0` |

---

## 总结

| 特性         | 实现关键点                         |
| ------------ | ---------------------------------- |
| 聚合类型传参 | 指针传递 + 被调用方拷贝            |
| SRET 优化    | 隐藏指针参数，直接写入             |
| 左值/右值    | `generating_lvalue_` 标志控制 load |
| 短路求值     | 条件跳转 + phi 节点合并            |
| Shadowing    | 作用域栈 + 新 alloca               |
| 零初始化     | 检测 + memset 优化                 |
| 字段对齐     | 插入 padding 满足对齐              |

---

**文档生成时间**: 2025-11-27
