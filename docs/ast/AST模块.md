# 抽象语法树(AST)模块详解

## AST 的哲学意义

源代码是一维的文本序列,但程序的结构是多维的、层次化的。`1 + 2 * 3`不是 7 个字符的列表,而是一个**树形结构**:

```
    +
   / \
  1   *
     / \
    2   3
```

抽象语法树(AST)是**代码结构的显式表示**,将隐含在文本中的层次关系变成数据结构。

### 为何称为"抽象"?

相对于**具体语法树(Concrete Syntax Tree, CST)**,AST 是"抽象的":

CST 包含所有语法细节:

```
Statement
├─ LetKeyword("let")
├─ Identifier("x")
├─ Equal("=")
├─ Number("42")
└─ Semicolon(";")
```

AST 只保留语义相关信息:

```
LetStmt
├─ name: "x"
└─ init: IntLiteral(42)
```

括号、分号、关键字等**语法糖**被去除,只保留程序的本质结构。这就是"抽象"的含义:**抽离语法噪音,保留语义本质**。

### AST 是编译器的通用语言

编译器的各个阶段都通过 AST 通信:

```
Parser生成AST → 名称解析标注AST → 类型检查填充AST → 代码生成遍历AST
```

AST 是**内存中的程序表示**,不依赖于文本格式,可以被任意分析和转换。这让编译器各阶段解耦——每个阶段只需理解 AST,不需理解源码文本。

## 节点层次的设计哲学

### 为何需要多个基类?

AST 有四大节点类别:`Expr`, `Stmt`, `Item`, `TypeNode`。为什么不用一个`Node`基类?

**类型安全和语义区分**。

看这个例子:

```rust
let x = if cond { 1 } else { 2 };  // if是表达式,有值
if cond { statement; }              // if是语句,无值(值被丢弃)
```

同样的`if`,在不同上下文有不同语义:

- 表达式上下文:必须返回值,两个分支类型统一
- 语句上下文:可以无返回值,值被忽略

用不同的基类(`Expr` vs `Stmt`)区分,让类型系统帮助我们:

```cpp
void process_expression(Expr* expr) {
    // 只能传入表达式,编译期检查
}

process_expression(new IfExpr(...));    // OK
process_expression(new LetStmt(...));   // 编译错误!
```

这是**静态类型的威力**:用类型系统避免混淆不同语义的节点。

### 表达式和语句的本质区别

| 方面     | 表达式(Expr)                      | 语句(Stmt)              |
| -------- | --------------------------------- | ----------------------- |
| 是否有值 | 有                                | 无(或值被丢弃)          |
| 可组合性 | 可嵌套                            | 不能嵌套                |
| 例子     | `1 + 2`, `f()`, `if { } else { }` | `let x = 1;`, `return;` |
| 类型     | 有意义的类型                      | Unit 类型`()`           |

Rust 的特点:**表达式为主**。很多其他语言的语句,在 Rust 中是表达式:

- `if`表达式可以返回值
- `match`表达式可以返回值
- 代码块`{ }`是表达式

这让代码更简洁:

```rust
let status = if x > 0 { "positive" } else { "non-positive" };
```

不需要:

```rust
let status;
if x > 0 {
    status = "positive";
} else {
    status = "non-positive";
}
```

AST 设计反映了这个特点:**IfExpr 继承自 Expr,而不是 Stmt**。

### 顶层项(Item)的独特性

`Item`代表模块级声明:函数、结构体、枚举、常量等。

为什么不是`Stmt`或`Expr`?

**作用域和生命周期不同**:

- 语句在函数内,执行时存在
- 表达式是语句的组成部分
- 顶层项在整个程序存在,定义全局符号

```rust
fn foo() { }        // Item:全局函数
const MAX: i32 = 100;  // Item:全局常量

fn main() {
    let x = 1;      // Stmt:局部变量
    foo();          // Expr in Stmt:函数调用
}
```

区分`Item`让编译器明确:**这些是全局定义,需要加入全局符号表**。

### TypeNode 的必要性

类型注解是程序的一部分,但不是表达式或语句:

```rust
let x: i32 = 42;
//     ^^^
//   类型注解
```

`i32`不能执行,不能求值,它是**元数据**——描述值的信息。

`TypeNode`捕获这个元数据,连接语法和类型系统:

- 解析阶段:构建`TypeNode`(语法层)
- 类型解析:将`TypeNode`转换为`Type`对象(语义层)

这个转换是必要的,因为:**类型系统需要比语法更丰富的信息**。例如数组类型`[i32; 10]`,TypeNode 包含表达式`10`,Type 对象包含求值后的数值`10`。

## 字段设计的深层考量

### type 字段:从无类型到有类型

每个`Expr`节点都有`type`字段:

```cpp
struct Expr {
    shared_ptr<Type> type;  // 初始为空,类型检查后填充
};
```

为什么不在构造时就确定类型?

**类型推导需要上下文**。

考虑:`let x = 42;`

`42`的类型是什么?不能在解析时确定,因为:

- 如果后续`let y: i32 = x;`,则`x`和`42`都是`i32`
- 如果后续`let y: u32 = x;`,则`x`和`42`都是`u32`

类型推导是**全局过程**,需要分析整个函数甚至模块。所以类型检查是独立阶段,在 AST 构建后执行。

`type`字段的演化:

1. 解析阶段:`type == nullptr`
2. 类型检查阶段:`type = 推导的类型`
3. 代码生成阶段:读取`type`,生成对应指令

这体现了**多遍编译**的思想:AST 是共享数据结构,不同阶段逐步填充信息。

### resolved_symbol:连接使用和定义

变量表达式`x`引用一个变量,但**引用哪个变量**?

```rust
let x = 1;
{
    let x = 2;
    print(x);  // 打印哪个x?
}
```

内层的`x`遮蔽外层的`x`,`print(x)`引用内层。

`resolved_symbol`记录这个绑定:

```cpp
struct VariableExpr : public Expr {
    string name;  // "x"
    shared_ptr<Symbol> resolved_symbol;  // 指向符号表条目
};
```

名称解析阶段填充`resolved_symbol`,将名字连接到定义:

```
VariableExpr("x").resolved_symbol → Symbol{name="x", type=i32, ...}
```

为什么不直接存储类型等信息?**避免重复,集中管理**。

符号表是中心化的数据库,所有变量引用都指向符号表条目。修改符号信息(如类型),所有引用自动"看到"更新,无需遍历 AST 修改每个节点。

### is_mutable_lvalue:可变性的传播

Rust 的可变性检查要求:**只有可变左值才能被赋值**。

```rust
let x = 1;
x = 2;  // 错误:x不可变

let mut y = 1;
y = 2;  // OK:y可变
```

`is_mutable_lvalue`字段追踪这个属性:

```cpp
struct Expr {
    bool is_mutable_lvalue;  // 是否为可变左值
};
```

**左值(Lvalue)**:表示内存位置的表达式(变量、字段、数组元素等)
**可变左值**:绑定是`mut`的左值

类型检查时,表达式向上传播可变性:

```cpp
// 变量表达式
VariableExpr("x").is_mutable_lvalue = 符号表查询x的is_mutable

// 字段访问
FieldExpr("obj.field").is_mutable_lvalue = obj.is_mutable_lvalue && field允许mut访问

// 解引用
UnaryExpr("*ptr").is_mutable_lvalue = ptr是&mut引用
```

赋值检查时,验证目标:

```cpp
void check_assignment(AssignmentExpr* node) {
    if (!node->target->is_mutable_lvalue) {
        error("Cannot assign to immutable expression");
    }
}
```

这个字段让**可变性成为 AST 的一部分**,类型检查器可以直接读取,无需重新计算。

### has_semicolon:表达式 vs 语句的转换

Rust 的分号有特殊语义:**表达式+分号=语句,值被丢弃**。

```rust
{
    42      // 块返回42
}

{
    42;     // 块返回(),值被丢弃
}
```

`has_semicolon`标记分号的存在:

```cpp
struct Expr {
    bool has_semicolon;  // 是否带分号
};
```

类型检查时,带分号的表达式强制为`Unit`类型:

```cpp
if (expr->has_semicolon) {
    expr->type = make_shared<UnitType>();  // 强制为()
}
```

这让 AST 准确反映源码语义:**同一个表达式,加不加分号,类型不同**。

## 字面量表达式的设计

### 为何保留 Token 而不是解析后的值?

```cpp
struct LiteralExpr : public Expr {
    Token literal;  // 保留Token,包含lexeme
};
```

为什么不直接存储值?

```cpp
struct IntLiteral : public Expr {
    long long value;  // 解析后的值
    string type_suffix;  // 类型后缀
};
```

**保留原始信息,延迟决策**。

Token 包含:

- `lexeme`:原始文本,如`"42i32"`, `"0xFF"`
- `type`:Token 类型,如`INTEGER_LITERAL_I32`
- `line`, `column`:位置信息

这些在后续阶段都有用:

- 错误报告:显示原始文本`"你写的0xFF"`
- 常量求值:从 lexeme 解析数值
- 代码生成:可能需要原始格式(如十六进制)

如果提前解析为值,丢失了原始信息。保留 Token,灵活性更高。

### 数组字面量 vs 数组初始化器

两种数组创建语法:

```rust
let a = [1, 2, 3];        // 数组字面量
let b = [0; 100];         // 数组初始化器
```

为什么用不同的 AST 节点?

**语义不同,后续处理不同**:

- `ArrayLiteralExpr`:包含每个元素的表达式,类型检查要验证所有元素类型一致
- `ArrayInitializerExpr`:包含单个值和计数,类型检查要验证计数是常量,常量求值要计算大小

用不同节点,让类型检查器分别处理,代码更清晰:

```cpp
class TypeCheckVisitor {
    void visit(ArrayLiteralExpr* node) {
        // 检查所有元素类型
        for (auto& elem : node->elements) {
            check(elem);
        }
    }

    void visit(ArrayInitializerExpr* node) {
        // 求值size,必须是常量
        auto size = const_eval(node->size);
        if (!size) error("Array size must be constant");
    }
};
```

这是**单一职责原则**在 AST 设计的体现:每种节点对应一种语义。

## 二元和一元表达式的统一

### 为何用一个 BinaryExpr 表示所有二元运算?

所有二元运算(`+`, `-`, `*`, `/`, `<`, `==`等)都用同一个节点:

```cpp
struct BinaryExpr : public Expr {
    shared_ptr<Expr> left;
    Token op;  // 运算符
    shared_ptr<Expr> right;
};
```

为什么不为每个运算符定义节点?

```cpp
struct AddExpr : public Expr { ... };
struct SubExpr : public Expr { ... };
struct MulExpr : public Expr { ... };
```

**权衡简洁性和类型安全**。

分开定义的优点:

- 类型系统强制检查:不会把加法误当乘法
- 每种运算可以有特定字段

统一定义的优点:

- AST 类定义简洁:一个类而不是 10+个类
- 处理逻辑统一:用 switch 而不是虚函数重载
- 灵活性高:添加新运算符只需修改处理逻辑,不修改 AST 定义

当前选择统一,因为:**简洁性和灵活性更重要**,类型安全由`Token.type`保证,而不是 C++类型系统。

### 运算符重载的处理

同一个运算符在不同类型有不同语义:

```rust
1 + 2          // 整数加法
ptr + 1        // 指针偏移
"hello" + " world"  // 字符串拼接(通过trait,当前未实现)
```

AST 只表示**语法结构**:`+`运算符和两个操作数。**语义**(具体做什么运算)由类型检查器根据操作数类型决定。

AST 阶段:

```
BinaryExpr(op=PLUS, left=ptr, right=1)
```

类型检查阶段:

```cpp
if (left_type是指针 && right_type是整数) {
    // 指针偏移语义
    node->type = left_type;
} else if (left_type是整数 && right_type是整数) {
    // 整数加法语义
    node->type = unified_int_type(left_type, right_type);
}
```

这体现了**语法和语义的分离**:AST 描述"写了什么",类型检查决定"意味着什么"。

**示例**: `[0; 10]` 创建 10 个 0 的数组
**要求**: size 必须是编译期常量

### 变量和标识符

#### VariableExpr - 变量引用

```cpp
struct VariableExpr : public Expr {
    Token name;  // 变量名Token

    explicit VariableExpr(Token name);
};
```

**示例**: `x`, `my_var`, `MAX_SIZE`
**解析**: resolved_symbol 指向符号表中的变量定义

#### UnderscoreExpr - 下划线表达式

```cpp
struct UnderscoreExpr : public Expr {
    Token underscore_token;

    explicit UnderscoreExpr(Token token);
};
```

**示例**: `_` (通配符,用于忽略值)
**用途**: match 模式、函数参数等

### 运算符表达式

#### UnaryExpr - 一元运算

```cpp
struct UnaryExpr : public Expr {
    Token op;                 // 运算符Token
    shared_ptr<Expr> right;   // 操作数

    UnaryExpr(Token op, shared_ptr<Expr> right);
};
```

**支持的运算符**:

- `-`: 取负 `-x`
- `+`: 取正 `+x`
- `!`: 逻辑非 `!flag`
- `*`: 解引用 `*ptr`

#### BinaryExpr - 二元运算

```cpp
struct BinaryExpr : public Expr {
    shared_ptr<Expr> left;   // 左操作数
    Token op;                 // 运算符Token
    shared_ptr<Expr> right;   // 右操作数

    BinaryExpr(shared_ptr<Expr> left, Token op, shared_ptr<Expr> right);
};
```

**支持的运算符**:

- 算术: `+`, `-`, `*`, `/`, `%`
- 比较: `==`, `!=`, `<`, `<=`, `>`, `>=`
- 逻辑: `&&`, `||`
- 位运算: `&`, `|`, `^`, `<<`, `>>`

**示例**: `x + y`, `a == b`, `n << 2`

### 调用和访问

#### CallExpr - 函数调用

```cpp
struct CallExpr : public Expr {
    shared_ptr<Expr> callee;             // 被调用的表达式
    vector<shared_ptr<Expr>> arguments;  // 参数列表

    CallExpr(shared_ptr<Expr> callee, vector<shared_ptr<Expr>> args);
};
```

**示例**: `print("hello")`, `add(1, 2)`, `obj.method(x)`
**callee**: 可以是变量、字段访问、路径表达式等

#### FieldAccessExpr - 字段访问

```cpp
struct FieldAccessExpr : public Expr {
    shared_ptr<Expr> object;  // 对象表达式
    Token field;              // 字段名Token

    FieldAccessExpr(shared_ptr<Expr> obj, Token fld);
};
```

**示例**: `obj.field`, `point.x`, `person.name`
**类型检查**: 验证 object 类型是否有该字段

#### IndexExpr - 索引访问

```cpp
struct IndexExpr : public Expr {
    shared_ptr<Expr> object;  // 被索引的对象(数组、切片)
    shared_ptr<Expr> index;   // 索引表达式

    IndexExpr(shared_ptr<Expr> obj, shared_ptr<Expr> idx);
};
```

**示例**: `arr[0]`, `matrix[i][j]`
**类型**: object 必须是数组或切片类型

### 赋值表达式

#### AssignmentExpr - 赋值

```cpp
struct AssignmentExpr : public Expr {
    shared_ptr<Expr> target;  // 赋值目标(左值)
    shared_ptr<Expr> value;   // 赋值值

    AssignmentExpr(shared_ptr<Expr> t, shared_ptr<Expr> v);
};
```

**示例**: `x = 10`, `arr[0] = value`
**检查**: target 必须是可变左值

#### CompoundAssignmentExpr - 复合赋值

```cpp
struct CompoundAssignmentExpr : public Expr {
    shared_ptr<Expr> target;  // 赋值目标
    Token op;                 // 复合运算符
    shared_ptr<Expr> value;   // 值

    CompoundAssignmentExpr(shared_ptr<Expr> t, Token o, shared_ptr<Expr> v);
};
```

**示例**: `x += 5`, `n *= 2`, `flags |= FLAG`
**等价**: `x += 5` ≈ `x = x + 5`

### 控制流表达式

#### IfExpr - if 表达式

```cpp
struct IfExpr : public Expr {
    shared_ptr<Expr> condition;                  // 条件表达式
    shared_ptr<Expr> then_branch;                // then分支
    optional<shared_ptr<Expr>> else_branch;      // else分支(可选)

    IfExpr(shared_ptr<Expr> cond, shared_ptr<Expr> then_b,
           optional<shared_ptr<Expr>> else_b, bool has_semi = false);
};
```

**示例**:

```rust
if x > 0 { 1 } else { -1 }
if flag { do_something(); }
```

**作为表达式**:

- 有 else 分支且无分号 → 返回值
- 无 else 或有分号 → 返回 Unit

#### LoopExpr - 无限循环

```cpp
struct LoopExpr : public Expr {
    shared_ptr<Stmt> body;  // 循环体

    LoopExpr(shared_ptr<Stmt> body);
};
```

**示例**: `loop { ... }`
**退出**: 通过 break 语句

#### WhileExpr - while 循环

```cpp
struct WhileExpr : public Expr {
    shared_ptr<Expr> condition;  // 循环条件
    shared_ptr<Stmt> body;       // 循环体

    WhileExpr(shared_ptr<Expr> cond, shared_ptr<Stmt> body);
};
```

**示例**: `while x < 10 { x += 1; }`

#### MatchExpr - 模式匹配

```cpp
struct MatchExpr : public Expr {
    shared_ptr<Expr> scrutinee;              // 被匹配的值
    vector<shared_ptr<MatchArm>> arms;       // 匹配分支

    MatchExpr(shared_ptr<Expr> scrut, vector<shared_ptr<MatchArm>> arms_vec);
};

struct MatchArm : public Node {
    shared_ptr<Pattern> pattern;              // 模式
    optional<shared_ptr<Expr>> guard;         // 守卫条件(可选)
    shared_ptr<Expr> body;                    // 分支体
};
```

**示例**:

```rust
match x {
    0 => "zero",
    1 | 2 => "one or two",
    n if n > 10 => "big",
    _ => "other",
}
```

### 类型转换和引用

#### AsExpr - 类型转换

```cpp
struct AsExpr : public Expr {
    shared_ptr<Expr> expression;    // 被转换的表达式
    shared_ptr<TypeNode> target_type; // 目标类型

    AsExpr(shared_ptr<Expr> expr, shared_ptr<TypeNode> type);
};
```

**示例**: `x as i32`, `ptr as usize`
**检查**: 验证转换是否合法

#### ReferenceExpr - 取引用

```cpp
struct ReferenceExpr : public Expr {
    bool is_mutable;               // 是否可变引用
    shared_ptr<Expr> expression;   // 被引用的表达式

    ReferenceExpr(bool is_mut, shared_ptr<Expr> expr);
};
```

**示例**: `&x` (不可变引用), `&mut y` (可变引用)
**类型**: `&T` 或 `&mut T`

### 复合表达式

#### GroupingExpr - 分组表达式

```cpp
struct GroupingExpr : public Expr {
    shared_ptr<Expr> expression;

    explicit GroupingExpr(shared_ptr<Expr> expr);
};
```

**示例**: `(x + y)`, `((a))`
**作用**: 改变运算优先级

#### TupleExpr - 元组表达式

```cpp
struct TupleExpr : public Expr {
    vector<shared_ptr<Expr>> elements;

    explicit TupleExpr(vector<shared_ptr<Expr>> elems);
};
```

**示例**: `(1, 2)`, `(x, "hello", true)`
**类型**: `(T1, T2, ...)`

#### UnitExpr - 单元表达式

```cpp
struct UnitExpr : public Expr {
    // 无字段
};
```

**示例**: `()`
**类型**: Unit 类型 `()`

#### BlockExpr - 块表达式

```cpp
struct BlockExpr : public Expr {
    shared_ptr<BlockStmt> block_stmt;

    explicit BlockExpr(shared_ptr<BlockStmt> stmt);
};
```

**示例**: `{ let x = 1; x + 1 }`
**值**: 最后一个表达式的值(无分号时)

### 结构体和路径

#### StructInitializerExpr - 结构体初始化

```cpp
struct StructInitializerExpr : public Expr {
    shared_ptr<Expr> name;                           // 结构体名(可能是路径)
    vector<shared_ptr<FieldInitializer>> fields;     // 字段初始化列表

    StructInitializerExpr(shared_ptr<Expr> n,
                          vector<shared_ptr<FieldInitializer>> f);
};

struct FieldInitializer : public Node {
    Token name;              // 字段名
    shared_ptr<Expr> value;  // 字段值
};
```

**示例**:

```rust
Point { x: 10, y: 20 }
Person { name: "Alice", age: 30 }
```

#### PathExpr - 路径表达式

```cpp
struct PathExpr : public Expr {
    shared_ptr<Expr> left;   // 左侧(模块、类型等)
    Token op;                // :: 运算符
    shared_ptr<Expr> right;  // 右侧(项名)

    PathExpr(shared_ptr<Expr> l, Token o, shared_ptr<Expr> r);
};
```

**示例**: `std::io::stdin`, `MyEnum::Variant`, `String::from`
**用途**: 访问模块项、枚举变体、关联函数等

#### ReturnExpr - return 表达式

```cpp
struct ReturnExpr : public Expr {
    shared_ptr<ReturnStmt> return_stmt;

    explicit ReturnExpr(shared_ptr<ReturnStmt> stmt);
};
```

**示例**: `return 42`, `return`

## 语句节点详解

### BlockStmt - 块语句

```cpp
struct BlockStmt : public Stmt {
    bool has_semicolon = false;
    vector<shared_ptr<Stmt>> statements;         // 语句列表
    optional<shared_ptr<Expr>> final_expr;       // 最终表达式(可选)
};
```

**示例**:

```rust
{
    let x = 1;
    let y = 2;
    x + y  // final_expr
}
```

**作为表达式**: 有 final_expr 且无分号时返回 final_expr 的值

### ExprStmt - 表达式语句

```cpp
struct ExprStmt : public Stmt {
    shared_ptr<Expr> expression;
    bool has_semicolon;

    explicit ExprStmt(shared_ptr<Expr> expr, bool has_semi = false);
};
```

**示例**: `print("hi");`, `x + y;`
**分号**: 有分号则丢弃值,无分号可作为块的返回值

### LetStmt - let 绑定

```cpp
struct LetStmt : public Stmt {
    shared_ptr<Pattern> pattern;                        // 绑定模式
    optional<shared_ptr<TypeNode>> type_annotation;     // 类型注解(可选)
    optional<shared_ptr<Expr>> initializer;             // 初始化器(可选)

    LetStmt(shared_ptr<Pattern> pat,
            optional<shared_ptr<TypeNode>> type_ann,
            optional<shared_ptr<Expr>> init);
};
```

**示例**:

```rust
let x: i32 = 42;
let (a, b) = (1, 2);
let mut count;
```

### ReturnStmt - return 语句

```cpp
struct ReturnStmt : public Stmt {
    Token keyword;
    optional<shared_ptr<Expr>> value;  // 返回值(可选)

    ReturnStmt(Token keyword, optional<shared_ptr<Expr>> val);
};
```

**示例**: `return 42;`, `return;`

### BreakStmt - break 语句

```cpp
struct BreakStmt : public Stmt {
    optional<shared_ptr<Expr>> value;  // 循环返回值(可选)

    BreakStmt(optional<shared_ptr<Expr>> value);
};
```

**示例**: `break;`, `break result;`

### ContinueStmt - continue 语句

```cpp
struct ContinueStmt : public Stmt {
    // 无字段
};
```

**示例**: `continue;`

### ItemStmt - 项语句

```cpp
struct ItemStmt : public Stmt {
    shared_ptr<Item> item;

    explicit ItemStmt(shared_ptr<Item> i);
};
```

**作用**: 在函数内部声明项(如嵌套函数)

## 顶层项节点详解

### FnDecl - 函数声明

```cpp
struct FnDecl : public Item {
    Token name;                                        // 函数名
    vector<shared_ptr<FnParam>> params;                // 参数列表
    optional<shared_ptr<TypeNode>> return_type;        // 返回类型(可选)
    optional<shared_ptr<BlockStmt>> body;              // 函数体(可选)
};

struct FnParam : public Node {
    shared_ptr<Pattern> pattern;   // 参数模式
    shared_ptr<TypeNode> type;     // 参数类型
};
```

**示例**:

```rust
fn add(x: i32, y: i32) -> i32 {
    x + y
}

fn main() {
    println!("Hello");
}
```

### StructDecl - 结构体声明

```cpp
struct StructDecl : public Item {
    Token name;
    StructKind kind;  // Normal, Tuple, Unit
    vector<shared_ptr<Field>> fields;            // 普通结构体字段
    vector<shared_ptr<TypeNode>> tuple_fields;   // 元组结构体字段
};

struct Field : public Node {
    Token name;
    shared_ptr<TypeNode> type;
};

enum class StructKind { Normal, Tuple, Unit };
```

**示例**:

```rust
struct Point { x: i32, y: i32 }  // Normal
struct Color(u8, u8, u8);         // Tuple
struct Marker;                     // Unit
```

### ConstDecl - 常量声明

```cpp
struct ConstDecl : public Item {
    Token name;
    shared_ptr<TypeNode> type;
    shared_ptr<Expr> value;

    ConstDecl(Token n, shared_ptr<TypeNode> t, shared_ptr<Expr> v);
};
```

**示例**: `const MAX: i32 = 100;`

### EnumDecl - 枚举声明

```cpp
struct EnumDecl : public Item {
    Token name;
    vector<shared_ptr<EnumVariant>> variants;
};

struct EnumVariant : public Node {
    Token name;
    EnumVariantKind kind;  // Plain, Tuple, Struct
    optional<shared_ptr<Expr>> discriminant;     // 判别值(可选)
    vector<shared_ptr<TypeNode>> tuple_types;    // 元组变体类型
    vector<shared_ptr<Field>> fields;            // 结构体变体字段
};

enum class EnumVariantKind { Plain, Tuple, Struct };
```

**示例**:

```rust
enum Option {
    None,
    Some(i32),
}

enum Message {
    Quit,
    Move { x: i32, y: i32 },
}
```

### ModDecl - 模块声明

```cpp
struct ModDecl : public Item {
    Token name;
    vector<shared_ptr<Item>> items;  // 模块内的项
};
```

**示例**:

```rust
mod utils {
    fn helper() { }
}
```

### TraitDecl - trait 声明

```cpp
struct TraitDecl : public Item {
    Token name;
    vector<shared_ptr<FnDecl>> methods;  // trait方法
};
```

**示例**:

```rust
trait Display {
    fn display(&self);
}
```

### ImplBlock - impl 块

```cpp
struct ImplBlock : public Item {
    shared_ptr<TypeNode> type;                // 实现的类型
    optional<shared_ptr<TypeNode>> trait_ref; // trait引用(可选)
    vector<shared_ptr<FnDecl>> methods;       // 方法实现
};
```

**示例**:

```rust
impl Point {
    fn new(x: i32, y: i32) -> Point {
        Point { x, y }
    }
}
```

## 类型节点详解

### TypeNameNode - 类型名

```cpp
struct TypeNameNode : public TypeNode {
    Token name;

    explicit TypeNameNode(Token name);
};
```

**示例**: `i32`, `String`, `MyType`

### ArrayTypeNode - 数组类型

```cpp
struct ArrayTypeNode : public TypeNode {
    shared_ptr<TypeNode> element_type;
    shared_ptr<Expr> size;  // 常量表达式
};
```

**示例**: `[i32; 10]`, `[[u8; 3]; 5]`

### TupleTypeNode - 元组类型

```cpp
struct TupleTypeNode : public TypeNode {
    vector<shared_ptr<TypeNode>> elements;
};
```

**示例**: `(i32, String)`, `(f64, f64, f64)`

### UnitTypeNode - 单元类型

```cpp
struct UnitTypeNode : public TypeNode {
    // 无字段
};
```

**示例**: `()`

### ReferenceTypeNode - 引用类型

```cpp
struct ReferenceTypeNode : public TypeNode {
    bool is_mutable;
    shared_ptr<TypeNode> referenced_type;
};
```

**示例**: `&i32`, `&mut String`

### RawPointerTypeNode - 裸指针类型

```cpp
struct RawPointerTypeNode : public TypeNode {
    bool is_mutable;
    shared_ptr<TypeNode> pointee_type;
};
```

**示例**: `*const i32`, `*mut u8`

### SliceTypeNode - 切片类型

```cpp
struct SliceTypeNode : public TypeNode {
    shared_ptr<TypeNode> element_type;
};
```

**示例**: `[i32]`, `[u8]`

### PathTypeNode - 路径类型

```cpp
struct PathTypeNode : public TypeNode {
    shared_ptr<Expr> path;
    optional<vector<shared_ptr<TypeNode>>> generic_args;
};
```

**示例**: `Vec<i32>`, `Option<String>`, `std::collections::HashMap`

### SelfTypeNode - Self 类型

```cpp
struct SelfTypeNode : public TypeNode {
    // 无字段
};
```

**示例**: `Self` (在 impl 块中表示当前类型)

## 模式节点详解

### IdentifierPattern - 标识符模式

```cpp
struct IdentifierPattern : public Pattern {
    Token name;
    bool is_mutable;
    shared_ptr<Symbol> resolved_symbol;
};
```

**示例**: `x`, `mut count`

### WildcardPattern - 通配符模式

```cpp
struct WildcardPattern : public Pattern {
    // 无字段
};
```

**示例**: `_`

### LiteralPattern - 字面量模式

```cpp
struct LiteralPattern : public Pattern {
    Token literal;
};
```

**示例**: `42`, `"hello"`, `true`

### TuplePattern - 元组模式

```cpp
struct TuplePattern : public Pattern {
    vector<shared_ptr<Pattern>> elements;
};
```

**示例**: `(x, y)`, `(_, b, _)`

### SlicePattern - 切片模式

```cpp
struct SlicePattern : public Pattern {
    vector<shared_ptr<Pattern>> elements;
};
```

**示例**: `[a, b, c]`, `[first, .., last]`

### StructPattern - 结构体模式

```cpp
struct StructPattern : public Pattern {
    shared_ptr<Expr> path;
    vector<shared_ptr<StructPatternField>> fields;
    bool has_rest;  // 是否有..
};

struct StructPatternField : public Node {
    Token name;
    optional<shared_ptr<Pattern>> pattern;
};
```

**示例**: `Point { x, y }`, `Person { name, .. }`

### ReferencePattern - 引用模式

```cpp
struct ReferencePattern : public Pattern {
    bool is_mutable;
    shared_ptr<Pattern> pattern;
};
```

**示例**: `&x`, `&mut y`

### RestPattern - 剩余模式

```cpp
struct RestPattern : public Pattern {
    // 无字段
};
```

**示例**: `..` (在切片或元组模式中)

## 访问者模式

### 访问者接口(visit.h)

```cpp
template <typename R> class ExprVisitor {
  public:
    virtual R visit(LiteralExpr *node) = 0;
    virtual R visit(VariableExpr *node) = 0;
    virtual R visit(BinaryExpr *node) = 0;
    // ... 所有Expr子类的visit方法
};

class StmtVisitor {
  public:
    virtual void visit(BlockStmt *node) = 0;
    virtual void visit(ExprStmt *node) = 0;
    virtual void visit(LetStmt *node) = 0;
    // ... 所有Stmt子类的visit方法
};

// 类似的ItemVisitor, TypeVisitor, PatternVisitor
```

### accept 方法实现

```cpp
template <typename R> R Expr::accept(ExprVisitor<R> *visitor) {
    // 通过dynamic_cast确定具体类型,调用对应的visit方法
    if (auto *node = dynamic_cast<LiteralExpr *>(this))
        return visitor->visit(node);
    if (auto *node = dynamic_cast<VariableExpr *>(this))
        return visitor->visit(node);
    // ... 所有Expr子类
}
```

**设计优势**:

- 分离数据结构和算法
- 易于添加新的遍历操作(新 Visitor)
- 类型安全的多态分发

## AST 的使用

### 构建(Parser)

Parser 在语法分析时构建 AST:

```cpp
auto expr = make_shared<BinaryExpr>(
    make_shared<LiteralExpr>(Token(NUMBER, "1")),
    Token(PLUS, "+"),
    make_shared<LiteralExpr>(Token(NUMBER, "2"))
);
```

### 遍历(Semantic)

语义分析器使用 Visitor 遍历 AST:

```cpp
class TypeCheckVisitor : public ExprVisitor<shared_ptr<Symbol>> {
    shared_ptr<Symbol> visit(BinaryExpr *node) override {
        node->left->accept(this);   // 递归访问左子树
        node->right->accept(this);  // 递归访问右子树
        // 类型检查逻辑...
    }
};
```

### 打印(Debug)

每个节点实现 print 方法,用于调试:

```cpp
void BinaryExpr::print(ostream &os, int indent) const {
    os << string(indent, ' ') << "BinaryExpr " << op.lexeme << "\n";
    left->print(os, indent + 2);
    right->print(os, indent + 2);
}
```

## 设计亮点

### 1. 智能指针管理

使用`shared_ptr`自动管理内存,避免手动 delete 和内存泄漏。

### 2. optional 表示可选字段

使用`optional<T>`表示可选的 AST 部分,如 else 分支、返回值等,类型安全且语义清晰。

### 3. 类型标注字段

每个 Expr 有 type 字段,每个 TypeNode 有 resolved_type 字段,连接语法和语义。

### 4. 位置信息保留

Token 中包含行列号,错误报告时能准确定位。

### 5. 访问者模式

解耦数据结构和算法,易于扩展新的分析 Pass。

## 总结

AST 是编译器的核心数据结构,精心设计的节点层次和字段组织,使得代码结构清晰,易于理解和扩展。通过 visit.h 的访问者接口,可以方便地实现各种遍历操作:名称解析、类型检查、代码生成等。理解 AST 的设计,是掌握编译器架构的关键。
