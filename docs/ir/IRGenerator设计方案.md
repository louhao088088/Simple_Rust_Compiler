# IRGenerator 设计方案

## 一、核心架构设计

### 1.1 类结构

```cpp
class IRGenerator {
private:
    IREmitter emitter_;
    TypeMapper type_mapper_;
    ValueManager value_manager_;

    // 表达式结果存储优化
    std::map<ExpressionNode*, std::string> expr_results_;  // 表达式节点 → IR变量名

    int if_counter_ = 0;
    int while_counter_ = 0;
    int for_counter_ = 0;

public:
    IRGenerator();

    // 主入口
    std::string generate(ProgramNode* ast);

    // 访问者模式接口
    void visit_program(ProgramNode* node);
    void visit_function_def(FunctionDefNode* node);
    void visit_struct_def(StructDefNode* node);

    // 语句访问
    void visit_let_stmt(LetStatementNode* node);
    void visit_assignment_stmt(AssignmentNode* node);
    void visit_return_stmt(ReturnStatementNode* node);
    void visit_if_stmt(IfStatementNode* node);          // TODO: 控制流
    void visit_while_stmt(WhileStatementNode* node);    // TODO: 控制流
    void visit_block(BlockNode* node);

    // 表达式访问 - 存储结果而不是返回
    void visit_expr(ExpressionNode* node);
    void visit_literal(LiteralNode* node);
    void visit_identifier(IdentifierNode* node);
    void visit_binary_expr(BinaryExprNode* node);
    void visit_unary_expr(UnaryExprNode* node);
    void visit_call_expr(CallExprNode* node);
    void visit_array_access(ArrayAccessNode* node);
    void visit_field_access(FieldAccessNode* node);
    void visit_struct_init(StructInitNode* node);
    void visit_array_init(ArrayInitNode* node);

    // 辅助方法
    std::string get_expr_result(ExpressionNode* node);  // 获取存储的表达式结果
    void store_expr_result(ExpressionNode* node, const std::string& ir_var);
};
```

### 1.2 表达式结果存储优化

**设计理念**:

- 不通过返回值传递结果
- 不使用栈结构
- 在 `expr_results_` map 中存储每个表达式的计算结果
- 需要时直接查找

**示例**:

```cpp
void IRGenerator::visit_binary_expr(BinaryExprNode* node) {
    // 先计算左右操作数（会自动存储到 expr_results_）
    visit_expr(node->left);
    visit_expr(node->right);

    // 从存储位置获取结果
    std::string left_var = get_expr_result(node->left);
    std::string right_var = get_expr_result(node->right);

    // 生成二元运算
    std::string type_str = type_mapper_.map_type(node->resolved_type);
    std::string result = emitter_.emit_binary_op(
        node->op.lexeme, type_str, left_var, right_var
    );

    // 存储当前表达式的结果
    store_expr_result(node, result);
}

std::string IRGenerator::get_expr_result(ExpressionNode* node) {
    auto it = expr_results_.find(node);
    assert(it != expr_results_.end() && "Expression result not found!");
    return it->second;
}

void IRGenerator::store_expr_result(ExpressionNode* node, const std::string& ir_var) {
    expr_results_[node] = ir_var;
}
```

---

## 二、类型信息获取方案 ✅ 无需修改！

### 2.1 重大发现：AST 已经有完整的类型信息！

经过详细检查，发现 **AST 已经完美支持类型信息存储**，并且**语义分析阶段已经正确填充所有类型**！

**AST 现状**：

```cpp
// src/ast/ast.h (已存在)
struct Expr : public Node {
    std::shared_ptr<Type> type;              // ✅ 已有类型字段！
    std::shared_ptr<Symbol> resolved_symbol; // ✅ 已有符号字段！

    bool return_over = false;
    bool has_semicolon = false;
    bool is_mutable_lvalue = false;          // ✅ 已有可变性标记！

    template <typename R> R accept(ExprVisitor<R> *visitor);
};
```

**所有表达式节点自动继承**：

- `LiteralExpr` ✅
- `BinaryExpr` ✅
- `UnaryExpr` ✅
- `VariableExpr` ✅
- `CallExpr` ✅
- `IndexExpr` ✅
- `FieldAccessExpr` ✅
- `StructInitializerExpr` ✅
- ... 等等所有表达式

### 2.2 语义分析已正确填充类型

**已验证的填充位置**（共 24 个访问方法）：

```cpp
// src/semantic/type_check.cpp

// 字面量
std::shared_ptr<Symbol> TypeCheckVisitor::visit(LiteralExpr *node) {
    node->type = builtin_types_.i32_type;  // ✅
}

// 二元运算
std::shared_ptr<Symbol> TypeCheckVisitor::visit(BinaryExpr *node) {
    node->left->accept(this);
    node->right->accept(this);
    // ... 类型推导
    node->type = left_type;  // ✅
}

// 数组访问
std::shared_ptr<Symbol> TypeCheckVisitor::visit(IndexExpr *node) {
    auto array_type = std::dynamic_pointer_cast<ArrayType>(object_type);
    node->type = array_type->element_type;  // ✅
}

// 结构体初始化
std::shared_ptr<Symbol> TypeCheckVisitor::visit(StructInitializerExpr *node) {
    node->type = struct_type;  // ✅
}

// ... 所有 24 个表达式类型都已填充
```

### 2.3 IRGenerator 可以直接使用

**无需任何修改，直接使用 `node->type`**：

```cpp
void IRGenerator::visit_binary_expr(BinaryExpr* node) {
    visit_expr(node->left);
    visit_expr(node->right);

    std::string left_var = get_expr_result(node->left);
    std::string right_var = get_expr_result(node->right);

    // ✅ 直接使用 node->type，无需从其他地方获取！
    std::string type_str = type_mapper_.map_type(node->type);

    std::string result = emitter_.emit_binary_op(
        node->op.lexeme, type_str, left_var, right_var
    );

    store_expr_result(node, result);
}
```

### 2.4 优势总结

✅ **不需要修改 AST 结构**
✅ **不需要修改语义分析代码**
✅ **不需要重新运行 semantic_benchmark**
✅ **类型信息完整且经过测试（222/222 通过）**
✅ **可以立即开始 IRGenerator 实现**

详细分析请参考：`docs/ir/AST类型字段现状分析.md`

---

## 三、参数处理策略

### 3.1 方案 B: 参数通过 alloca 处理

**设计决策**: 函数参数在进入函数时立即 alloca + store，统一通过 ValueManager 管理

**生成代码示例**:

Rust 代码:

```rust
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}
```

生成的 LLVM IR:

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
    ; 参数 alloca
    %a_addr = alloca i32
    store i32 %a, i32* %a_addr

    %b_addr = alloca i32
    store i32 %b, i32* %b_addr

    ; 注册到 ValueManager
    ; value_manager_.define_variable("a", "%a_addr", "i32*", false);
    ; value_manager_.define_variable("b", "%b_addr", "i32*", false);

    ; 使用变量
    %0 = load i32, i32* %a_addr
    %1 = load i32, i32* %b_addr
    %2 = add i32 %0, %1
    ret i32 %2
}
```

**实现代码**:

```cpp
void IRGenerator::visit_function_def(FunctionDefNode* node) {
    // 1. 生成函数签名
    std::string ret_type_str = type_mapper_.map_type(node->return_type);
    // ... 参数类型

    emitter_.emit_function_def(ret_type_str, node->name.lexeme, param_types);
    emitter_.emit_label("entry");

    // 2. 进入函数作用域
    value_manager_.enter_scope();

    // 3. 处理参数: alloca + store + 注册
    for (size_t i = 0; i < node->parameters.size(); ++i) {
        auto& param = node->parameters[i];
        std::string param_name = param.name.lexeme;
        std::string param_ir_name = "%" + param_name;  // 函数参数名

        // 获取参数类型
        std::string param_type_str = type_mapper_.map_type(param.type);

        // 为参数创建 alloca
        std::string alloca_name = emitter_.emit_alloca(param_type_str);

        // 将参数值存入 alloca
        emitter_.emit_store(param_ir_name, alloca_name, param_type_str);

        // 注册到 ValueManager (注意: 存的是 alloca 的地址)
        value_manager_.define_variable(param_name, alloca_name,
                                       param_type_str + "*", false);
    }

    // 4. 生成函数体
    visit_block(node->body);

    // 5. 退出作用域
    value_manager_.exit_scope();

    emitter_.emit_close_function();
}
```

---

## 四、数组处理细化

### 4.1 数组类型映射

已在 TypeMapper 中实现：

```cpp
[i32; 10] → "[10 x i32]"
[bool; 5] → "[5 x i1]"
[[i32; 3]; 2] → "[2 x [3 x i32]]"
```

### 4.2 数组初始化

#### 场景 1: 字面量初始化

```rust
let arr = [1, 2, 3, 4, 5];
```

**生成策略**:

```llvm
; 1. 分配数组空间
%arr = alloca [5 x i32]

; 2. 逐个初始化元素
%0 = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 0
store i32 1, i32* %0

%1 = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 1
store i32 2, i32* %1

%2 = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 2
store i32 3, i32* %2

; ... 继续 4, 5
```

**实现代码**:

```cpp
void IRGenerator::visit_array_init(ArrayInitNode* node) {
    // 获取数组类型
    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->resolved_type);
    std::string array_type_str = type_mapper_.map_type(array_type);
    std::string elem_type_str = type_mapper_.map_type(array_type->element_type);

    // 分配数组空间
    std::string array_ptr = emitter_.emit_alloca(array_type_str);

    // 逐个初始化元素
    for (size_t i = 0; i < node->elements.size(); ++i) {
        // 计算元素的结果
        visit_expr(node->elements[i]);
        std::string elem_value = get_expr_result(node->elements[i]);

        // 获取元素地址: getelementptr
        std::string elem_ptr = emitter_.emit_getelementptr(
            array_type_str,      // [5 x i32]
            array_ptr,           // %arr
            {0, static_cast<int>(i)}  // 索引
        );

        // 存储元素值
        emitter_.emit_store(elem_value, elem_ptr, elem_type_str);
    }

    // 存储数组的结果（数组的指针）
    store_expr_result(node, array_ptr);
}
```

#### 场景 2: 重复值初始化

```rust
let arr = [0; 10];  // 10个0
```

**生成策略**:

```llvm
%arr = alloca [10 x i32]

; 循环初始化 (或展开)
%0 = getelementptr [10 x i32], [10 x i32]* %arr, i32 0, i32 0
store i32 0, i32* %0
%1 = getelementptr [10 x i32], [10 x i32]* %arr, i32 0, i32 1
store i32 0, i32* %1
; ... 重复10次
```

**待讨论**:

- 小数组（<10）直接展开
- 大数组（>=10）生成循环？

### 4.3 数组访问

```rust
let x = arr[i];
```

**生成策略**:

```llvm
; 1. 查找数组变量
; arr_info = value_manager_.lookup_variable("arr")
; arr_ptr = arr_info->alloca_name  // %arr

; 2. 计算索引
; visit_expr(index_expr)
; index_var = get_expr_result(index_expr)  // %i_value

; 3. 获取元素地址
%elem_ptr = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 %i_value

; 4. 加载元素值
%x_value = load i32, i32* %elem_ptr
```

**实现代码**:

```cpp
void IRGenerator::visit_array_access(ArrayAccessNode* node) {
    // 1. 获取数组
    VariableInfo* arr_info = value_manager_.lookup_variable(node->array->name.lexeme);
    assert(arr_info != nullptr);

    // 2. 计算索引
    visit_expr(node->index);
    std::string index_var = get_expr_result(node->index);

    // 3. 获取数组类型
    auto array_type = std::dynamic_pointer_cast<ArrayType>(node->array->resolved_type);
    std::string array_type_str = type_mapper_.map_type(array_type);
    std::string elem_type_str = type_mapper_.map_type(array_type->element_type);

    // 4. getelementptr 获取元素地址
    std::string elem_ptr = emitter_.emit_getelementptr(
        array_type_str,
        arr_info->alloca_name,
        {0, index_var}  // 第一个0是解引用指针，第二个是数组索引
    );

    // 5. load 元素值
    std::string elem_value = emitter_.emit_load(elem_type_str, elem_ptr);

    // 6. 存储结果
    store_expr_result(node, elem_value);
}
```

### 4.4 数组赋值

```rust
arr[i] = value;
```

**生成策略**:

```llvm
; 1. 计算右值
; 2. 获取元素地址 (getelementptr)
; 3. store 到元素地址
%elem_ptr = getelementptr [5 x i32], [5 x i32]* %arr, i32 0, i32 %i_value
store i32 %value, i32* %elem_ptr
```

---

## 五、结构体处理细化

### 5.1 结构体类型映射

已在 TypeMapper 中实现，使用 `ordered_fields` 保证字段顺序：

```cpp
struct Point {
    x: i32,
    y: i32,
}

→ %Point = type { i32, i32 }
```

### 5.2 结构体定义生成

```rust
struct Point {
    x: i32,
    y: i32,
}
```

**生成策略**:

```llvm
%Point = type { i32, i32 }
```

**实现代码**:

```cpp
void IRGenerator::visit_struct_def(StructDefNode* node) {
    // 1. 从语义分析获取结构体类型
    auto struct_type = std::dynamic_pointer_cast<StructType>(node->resolved_type);

    // 2. 生成字段类型列表
    std::vector<std::string> field_types;
    for (const auto& [field_name, field_type] : struct_type->ordered_fields) {
        std::string field_type_str = type_mapper_.map_type(field_type);
        field_types.push_back(field_type_str);
    }

    // 3. 生成结构体定义
    emitter_.emit_struct_def(node->name.lexeme, field_types);
}
```

### 5.3 结构体初始化

#### 场景 1: 完整初始化

```rust
let p = Point { x: 10, y: 20 };
```

**生成策略**:

```llvm
; 1. 分配结构体空间
%p = alloca %Point

; 2. 初始化字段 x (索引0)
%0 = getelementptr %Point, %Point* %p, i32 0, i32 0
store i32 10, i32* %0

; 3. 初始化字段 y (索引1)
%1 = getelementptr %Point, %Point* %p, i32 0, i32 1
store i32 20, i32* %1
```

**实现代码**:

```cpp
void IRGenerator::visit_struct_init(StructInitNode* node) {
    // 1. 获取结构体类型
    auto struct_type = std::dynamic_pointer_cast<StructType>(node->resolved_type);
    std::string struct_name = struct_type->name;
    std::string struct_type_str = "%" + struct_name;

    // 2. 分配结构体空间
    std::string struct_ptr = emitter_.emit_alloca(struct_type_str);

    // 3. 建立字段名到索引的映射
    std::map<std::string, int> field_indices;
    for (size_t i = 0; i < struct_type->ordered_fields.size(); ++i) {
        field_indices[struct_type->ordered_fields[i].first] = i;
    }

    // 4. 初始化每个字段
    for (const auto& field_init : node->fields) {
        std::string field_name = field_init.name.lexeme;
        int field_index = field_indices[field_name];

        // 计算字段值
        visit_expr(field_init.value);
        std::string field_value = get_expr_result(field_init.value);

        // 获取字段地址
        std::string field_ptr = emitter_.emit_getelementptr(
            struct_type_str,
            struct_ptr,
            {0, field_index}
        );

        // 获取字段类型
        auto field_type = struct_type->ordered_fields[field_index].second;
        std::string field_type_str = type_mapper_.map_type(field_type);

        // 存储字段值
        emitter_.emit_store(field_value, field_ptr, field_type_str);
    }

    // 5. 存储结果
    store_expr_result(node, struct_ptr);
}
```

**关键点**: 必须按照 `ordered_fields` 的顺序确定字段索引！

#### 场景 2: 部分初始化（如果支持）

```rust
struct Point {
    x: i32,
    y: i32,
}

let p = Point { x: 10, ..Default::default() };
```

**策略**: 暂时不支持，要求所有字段都初始化

### 5.4 结构体字段访问

```rust
let x = p.x;
```

**生成策略**:

```llvm
; 1. 查找结构体变量
; p_info = value_manager_.lookup_variable("p")
; p_ptr = p_info->alloca_name  // %p

; 2. 获取字段索引 (从 ordered_fields)
; field_index = 0  // x 是第0个字段

; 3. getelementptr 获取字段地址
%field_ptr = getelementptr %Point, %Point* %p, i32 0, i32 0

; 4. load 字段值
%x_value = load i32, i32* %field_ptr
```

**实现代码**:

```cpp
void IRGenerator::visit_field_access(FieldAccessNode* node) {
    // 1. 获取结构体变量
    VariableInfo* struct_info = value_manager_.lookup_variable(node->object->name.lexeme);
    assert(struct_info != nullptr);

    // 2. 获取结构体类型
    auto struct_type = std::dynamic_pointer_cast<StructType>(node->object->resolved_type);
    std::string struct_type_str = "%" + struct_type->name;

    // 3. 查找字段索引
    std::string field_name = node->field.lexeme;
    int field_index = -1;
    std::shared_ptr<Type> field_type;

    for (size_t i = 0; i < struct_type->ordered_fields.size(); ++i) {
        if (struct_type->ordered_fields[i].first == field_name) {
            field_index = i;
            field_type = struct_type->ordered_fields[i].second;
            break;
        }
    }
    assert(field_index != -1);

    // 4. getelementptr 获取字段地址
    std::string field_ptr = emitter_.emit_getelementptr(
        struct_type_str,
        struct_info->alloca_name,
        {0, field_index}
    );

    // 5. load 字段值
    std::string field_type_str = type_mapper_.map_type(field_type);
    std::string field_value = emitter_.emit_load(field_type_str, field_ptr);

    // 6. 存储结果
    store_expr_result(node, field_value);
}
```

### 5.5 结构体字段赋值

```rust
p.x = 30;
```

**生成策略**:

```llvm
; 1. 计算右值
; 2. 获取字段地址 (getelementptr)
; 3. store 到字段地址
%field_ptr = getelementptr %Point, %Point* %p, i32 0, i32 0
store i32 30, i32* %field_ptr
```

---

## 六、控制流处理（TODO）

### 6.1 if 语句

```rust
if cond {
    // then block
} else {
    // else block
}
```

**标签命名策略**:

```llvm
if_cond_0:
    ; 条件判断
    br i1 %cond, label %if_then_0, label %if_else_0

if_then_0:
    ; then 分支
    br label %if_end_0

if_else_0:
    ; else 分支
    br label %if_end_0

if_end_0:
    ; 继续执行
```

**待细化**:

- 嵌套 if 的标签计数
- if 表达式返回值（需要 phi 节点）

### 6.2 while 循环

```rust
while cond {
    // loop body
}
```

**标签命名策略**:

```llvm
while_cond_0:
    ; 条件判断
    br i1 %cond, label %while_body_0, label %while_end_0

while_body_0:
    ; 循环体
    br label %while_cond_0

while_end_0:
    ; 继续执行
```

**待细化**:

- break/continue 支持
- 循环变量的 phi 节点

---

## 七、实现阶段划分

### 第一阶段: 基础框架 (最小可运行)

**目标**: 生成最简单的函数和变量

**实现内容**:

1. ✅ IRGenerator 类框架
2. ✅ 表达式结果存储机制
3. ✅ 字面量表达式 (整数、布尔)
4. ✅ 变量标识符表达式
5. ✅ 二元运算表达式 (+, -, \*, /)
6. ✅ let 语句
7. ✅ return 语句
8. ✅ 函数定义（参数 alloca 策略）
9. ✅ 简单函数调用

**测试用例**:

```rust
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() {
    let x = 10;
    let y = 20;
    let z = add(x, y);
    return z;
}
```

### 第二阶段: 数组和结构体

**实现内容**:

1. ✅ 数组初始化
2. ✅ 数组访问
3. ✅ 数组赋值
4. ✅ 结构体定义
5. ✅ 结构体初始化
6. ✅ 结构体字段访问
7. ✅ 结构体字段赋值

**测试用例**:

```rust
struct Point {
    x: i32,
    y: i32,
}

fn main() {
    let arr = [1, 2, 3];
    let x = arr[0];

    let p = Point { x: 10, y: 20 };
    let px = p.x;

    return px;
}
```

### 第三阶段: 控制流

**实现内容**:

1. ⏳ if/else 语句
2. ⏳ while 循环
3. ⏳ phi 节点处理

**测试用例**:

```rust
fn max(a: i32, b: i32) -> i32 {
    if a > b {
        return a;
    } else {
        return b;
    }
}

fn sum(n: i32) -> i32 {
    let s = 0;
    let i = 0;
    while i < n {
        s = s + i;
        i = i + 1;
    }
    return s;
}
```

---

## 八、待确认问题

### 🔴 问题 1: 数组重复初始化

```rust
let arr = [0; 100];  // 100个0
```

是否生成循环，还是直接展开 100 次 store？

**建议**:

- 小数组（< 10）展开
- 大数组（>= 10）生成循环

### 🟡 问题 2: 结构体部分初始化

Rust 允许：

```rust
let p2 = Point { x: 30, ..p1 };
```

是否支持？

**建议**: 暂不支持，第一阶段要求全部字段初始化

### 🟡 问题 3: 引用和借用

```rust
let x = 10;
let r = &x;      // 引用
let m = &mut x;  // 可变引用
```

是否需要特殊处理？

**建议**:

- 简单引用就是指针（已在 TypeMapper 中处理）
- 借用检查已在语义分析完成
- IR 生成时不需要额外处理

### 🟡 问题 4: AST 节点类型字段添加位置

需要在哪些节点添加 `resolved_type`？

**建议**: 只在 `ExpressionNode` 基类添加，所有表达式节点自动继承

---

## 九、下一步行动

### 立即行动:

1. **修改 AST**: 在 `ExpressionNode` 添加 `resolved_type` 字段
2. **修改语义分析**: 填充 `resolved_type` 字段
3. **运行 semantic_benchmark**: 确保不破坏现有测试

### 确认后开始:

4. **实现 IRGenerator 框架** (第一阶段)
5. **编写测试用例** (简单函数)
6. **逐步添加功能**

---

你对这个设计方案有什么意见吗？特别是：

1. 数组重复初始化的策略？
2. 结构体部分初始化是否支持？
3. AST 修改后需要重点检查哪些语义分析代码？
