# 语法分析器(Parser)模块详解

## 模块概述

Parser 将 Token 序列转换为抽象语法树(AST)。本实现采用 Pratt 解析算法(又称优先级爬升算法),优雅地处理运算符优先级和结合性,同时支持递归下降处理复杂的语法结构。Parser 是连接词法分析和语义分析的桥梁。

## 文件组成

- **parser.h**: Parser 类定义、优先级枚举
- **parser.cpp**: Pratt 解析器实现、递归下降解析

## Pratt 解析算法

### 核心思想

Pratt 解析器通过以下机制处理表达式:

1. **前缀解析函数(Prefix Parser)**: 处理前缀运算符和独立表达式(字面量、变量、一元运算符等)
2. **中缀解析函数(Infix Parser)**: 处理中缀运算符和后缀运算符(二元运算符、函数调用、数组索引等)
3. **优先级(Precedence)**: 每个运算符有一个优先级,用于决定子表达式的边界

### 优先级枚举

```cpp
enum Precedence {
    NONE,          // 0  - 最低优先级
    ASSIGNMENT,    // 1  - = += -= *= /= 等
    RANGE,         // 2  - .. ..=
    OR,            // 3  - ||
    AND,           // 4  - &&
    COMPARISON,    // 5  - == != < > <= >=
    BITWISE_OR,    // 6  - |
    BITWISE_XOR,   // 7  - ^
    BITWISE_AND,   // 8  - &
    SHIFT,         // 9  - << >>
    TERM,          // 10 - + -
    FACTOR,        // 11 - * / %
    AS,            // 12 - as
    UNARY,         // 13 - ! - * &
    CALL,          // 14 - . () []
    PATH           // 15 - ::
};
```

**设计**: 数值越大优先级越高,Rust 的运算符优先级表。

### Parser 类结构

```cpp
class Parser {
  private:
    const vector<Token> &tokens_;     // Token序列(只读引用)
    ErrorReporter &error_reporter_;   // 错误报告器
    size_t current_ = 0;              // 当前Token索引

    // Pratt解析器核心数据结构
    map<TokenType, PrefixParseFn> prefix_parsers_;  // 前缀解析函数表
    map<TokenType, InfixParseFn> infix_parsers_;    // 中缀解析函数表
    map<TokenType, Precedence> precedences_;        // 优先级表

  public:
    Parser(const vector<Token> &tokens, ErrorReporter &error_reporter);
    shared_ptr<Program> parse();  // 主入口
};
```

### 函数类型定义

```cpp
using PrefixParseFn = function<shared_ptr<Expr>()>;
using InfixParseFn = function<shared_ptr<Expr>(shared_ptr<Expr>)>;
```

**PrefixParseFn**: 无参数,返回新表达式
**InfixParseFn**: 接受左操作数,返回完整表达式

## Parser 初始化

### 注册前缀解析器

在构造函数中注册各种前缀表达式的解析函数:

#### 字面量

```cpp
register_prefix(TokenType::NUMBER, [this] {
    return make_shared<LiteralExpr>(previous());
});
register_prefix(TokenType::STRING, [this] {
    return make_shared<LiteralExpr>(previous());
});
register_prefix(TokenType::TRUE, [this] {
    return make_shared<LiteralExpr>(previous());
});
register_prefix(TokenType::FALSE, [this] {
    return make_shared<LiteralExpr>(previous());
});
register_prefix(TokenType::CHAR, [this] {
    return make_shared<LiteralExpr>(previous());
});
```

**逻辑**: 直接用 previous() Token 创建 LiteralExpr

#### 标识符

```cpp
register_prefix(TokenType::IDENTIFIER, [this]() -> shared_ptr<Expr> {
    if (previous().lexeme == "_") {
        return make_shared<UnderscoreExpr>(previous());
    } else {
        return make_shared<VariableExpr>(previous());
    }
});
```

**特殊处理**: `_`创建 UnderscoreExpr,其他创建 VariableExpr

#### 一元运算符

```cpp
register_prefix(TokenType::MINUS, [this] {
    auto op = previous();
    auto right = parse_expression(Precedence::UNARY);  // 右递归
    return make_shared<UnaryExpr>(op, move(right));
});

register_prefix(TokenType::BANG, [this] {
    auto op = previous();
    auto right = parse_expression(Precedence::UNARY);
    return make_shared<UnaryExpr>(op, move(right));
});

register_prefix(TokenType::STAR, [this] {  // 解引用
    Token op = previous();
    auto right = parse_expression(Precedence::UNARY);
    return make_shared<UnaryExpr>(op, move(right));
});
```

**关键**: 调用`parse_expression(UNARY)`解析右操作数,优先级为 UNARY 保证正确结合

#### 引用运算符

```cpp
register_prefix(TokenType::AMPERSAND, [this]() -> shared_ptr<Expr> {
    bool is_mutable = match({TokenType::MUT});  // 检查是否有mut关键字
    auto expr = parse_expression(Precedence::UNARY);
    return make_shared<ReferenceExpr>(is_mutable, move(expr));
});
```

**逻辑**: `&x` → ReferenceExpr(false, x), `&mut y` → ReferenceExpr(true, y)

#### 控制流表达式

```cpp
register_prefix(TokenType::IF, [this] { return parse_if_expression(); });
register_prefix(TokenType::WHILE, [this] { return parse_while_expression(); });
register_prefix(TokenType::LOOP, [this] { return parse_loop_expression(); });
register_prefix(TokenType::MATCH, [this] { return parse_match_expression(); });
register_prefix(TokenType::RETURN, [this] { return parse_return_expression(); });
```

**设计**: 复杂结构委托给专门的解析函数

#### 分组和元组

```cpp
register_prefix(TokenType::LEFT_PAREN, [this]() -> shared_ptr<Expr> {
    if (check(TokenType::RIGHT_PAREN)) {
        consume(TokenType::RIGHT_PAREN, "Unclosed unit literal.");
        return make_shared<UnitExpr>();  // ()
    }

    auto expr = parse_expression(Precedence::NONE);

    if (match({TokenType::COMMA})) {
        // 元组: (expr, ...)
        vector<shared_ptr<Expr>> elements;
        elements.push_back(move(expr));
        while (!check(TokenType::RIGHT_PAREN) && !is_at_end()) {
            elements.push_back(parse_expression(Precedence::NONE));
            if (!check(TokenType::RIGHT_PAREN)) {
                consume(TokenType::COMMA, "Expect ',' between tuple elements.");
            }
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' to close tuple.");
        return make_shared<TupleExpr>(move(elements));
    } else {
        // 分组: (expr)
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return make_shared<GroupingExpr>(move(expr));
    }
});
```

**歧义处理**:

- `()` → UnitExpr
- `(expr)` → GroupingExpr
- `(e1, e2, ...)` → TupleExpr

通过检查逗号区分分组和元组

#### 数组字面量

```cpp
register_prefix(TokenType::LEFT_BRACKET, [this]() -> shared_ptr<Expr> {
    if (check(TokenType::RIGHT_BRACKET)) {
        consume(TokenType::RIGHT_BRACKET, "Unclosed empty array literal.");
        return make_shared<ArrayLiteralExpr>(vector<shared_ptr<Expr>>{});
    }

    auto first_expr = parse_expression(Precedence::NONE);

    if (match({TokenType::SEMICOLON})) {
        // [value; count] - 初始化器
        auto count_expr = parse_expression(Precedence::NONE);
        consume(TokenType::RIGHT_BRACKET, "Expect ']'.");
        return make_shared<ArrayInitializerExpr>(move(first_expr), move(count_expr));
    } else {
        // [e1, e2, ...] - 字面量
        vector<shared_ptr<Expr>> elements;
        elements.push_back(move(first_expr));
        while (match({TokenType::COMMA})) {
            if (check(TokenType::RIGHT_BRACKET)) break;
            elements.push_back(parse_expression(Precedence::NONE));
        }
        consume(TokenType::RIGHT_BRACKET, "Expect ']'.");
        return make_shared<ArrayLiteralExpr>(move(elements));
    }
});
```

**歧义处理**:

- `[]` → 空数组
- `[e1, e2]` → 数组字面量
- `[value; count]` → 数组初始化器

通过分号区分

#### 块表达式

```cpp
register_prefix(TokenType::LEFT_BRACE, [this] {
    current_--;  // 回退,让parse_block_statement处理
    auto block = parse_block_statement();
    return make_shared<BlockExpr>(move(block));
});
```

**技巧**: current\_--回退 Token,因为 parse_block_statement 会重新消费`{`

### 注册中缀解析器

#### 二元运算符

```cpp
register_infix(TokenType::PLUS, Precedence::TERM, [this](auto left) {
    auto op = previous();
    auto right = parse_expression(Precedence::TERM);  // 左结合
    return make_shared<BinaryExpr>(move(left), op, move(right));
});

register_infix(TokenType::STAR, Precedence::FACTOR, [this](auto left) {
    auto op = previous();
    auto right = parse_expression(Precedence::FACTOR);
    return make_shared<BinaryExpr>(move(left), op, move(right));
});

// 类似注册: -, /, %, &, |, ^, <<, >>, ==, !=, <, <=, >, >=, &&, ||
```

**优先级**: 每个运算符注册时指定优先级,如 PLUS 是 TERM(10), STAR 是 FACTOR(11)

**左结合**: `parse_expression(当前优先级)`实现左结合,如`a + b + c`解析为`(a + b) + c`

#### as 运算符

```cpp
register_infix(TokenType::AS, Precedence::AS, [this](auto left) -> shared_ptr<Expr> {
    auto target_type = parse_type();
    if (!target_type) {
        report_error(peek(), "Expect a type after 'as' keyword.");
        return nullptr;
    }
    return make_shared<AsExpr>(move(left), move(target_type));
});
```

**特殊**: 右侧是类型,不是表达式

#### 函数调用

```cpp
register_infix(TokenType::LEFT_PAREN, Precedence::CALL, [this](auto callee) {
    vector<shared_ptr<Expr>> arguments;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            if (peek().type == TokenType::RIGHT_PAREN) break;
            arguments.push_back(parse_expression(Precedence::NONE));
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return make_shared<CallExpr>(move(callee), move(arguments));
});
```

**逻辑**: `callee(arg1, arg2, ...)`

#### 字段访问

```cpp
register_infix(TokenType::DOT, Precedence::CALL, [this](auto left) {
    Token field_name = consume(TokenType::IDENTIFIER, "Expect field name after '.'.");
    return make_shared<FieldAccessExpr>(move(left), field_name);
});
```

**逻辑**: `object.field`

#### 数组索引

```cpp
register_infix(TokenType::LEFT_BRACKET, Precedence::CALL, [this](auto left) {
    auto index_expr = parse_expression(Precedence::NONE);
    consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
    return make_shared<IndexExpr>(move(left), move(index_expr));
});
```

**逻辑**: `array[index]`

#### 赋值运算符

```cpp
register_infix(TokenType::EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
    auto value = parse_expression(Precedence::ASSIGNMENT);  // 右结合
    return make_shared<AssignmentExpr>(move(left), move(value));
});

register_infix(TokenType::PLUS_EQUAL, Precedence::ASSIGNMENT, [this](auto left) {
    auto op = previous();
    auto right = parse_expression(Precedence::ASSIGNMENT);
    return make_shared<CompoundAssignmentExpr>(move(left), op, move(right));
});

// 类似: -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
```

**右结合**: `a = b = c`解析为`a = (b = c)`

#### 路径运算符

```cpp
register_infix(TokenType::COLON_COLON, Precedence::PATH, [this](auto left) {
    Token op = previous();
    auto right_token = consume(TokenType::IDENTIFIER, "Expect identifier after '::'.");
    auto right = make_shared<VariableExpr>(right_token);
    return make_shared<PathExpr>(move(left), op, move(right));
});
```

**逻辑**: `std::io::stdin` → PathExpr(PathExpr(std, io), stdin)

#### 结构体初始化(后缀)

```cpp
register_infix(TokenType::LEFT_BRACE, Precedence::CALL, [this](shared_ptr<Expr> left) {
    current_--;  // 回退
    return parse_struct_initializer(move(left));
});
```

**用法**: `Point { x: 1, y: 2 }`,left 是`Point`

### 辅助函数

```cpp
void register_prefix(TokenType type, PrefixParseFn fn) {
    prefix_parsers_[type] = fn;
}

void register_infix(TokenType type, Precedence prec, InfixParseFn fn) {
    infix_parsers_[type] = fn;
    precedences_[type] = prec;
}

Precedence get_precedence(TokenType type) {
    auto it = precedences_.find(type);
    return (it != precedences_.end()) ? it->second : Precedence::NONE;
}
```

## 核心解析算法

### parse_expression() - Pratt 核心

```cpp
shared_ptr<Expr> Parser::parse_expression(Precedence precedence) {
    // 1. 查找前缀解析函数
    advance();  // 消费当前Token
    auto prefix_it = prefix_parsers_.find(previous().type);
    if (prefix_it == prefix_parsers_.end()) {
        report_error(previous(), "Expect expression.");
        return nullptr;
    }

    // 2. 调用前缀解析函数,得到左表达式
    auto left = prefix_it->second();  // PrefixParseFn()
    if (!left) return nullptr;

    // 3. 处理中缀运算符,只要优先级更高就继续
    while (precedence < get_precedence(peek().type)) {
        advance();  // 消费运算符Token
        auto infix_it = infix_parsers_.find(previous().type);
        if (infix_it == infix_parsers_.end()) {
            return left;  // 没有中缀解析器,返回left
        }
        left = infix_it->second(left);  // InfixParseFn(left)
        if (!left) return nullptr;
    }

    return left;
}
```

**算法详解**:

1. **前缀阶段**: 调用前缀解析器,获得初始表达式
2. **中缀循环**: 只要下一个运算符优先级高于当前优先级,就继续消费
3. **递归调用**: 中缀解析器内部调用`parse_expression(当前优先级)`解析右侧

**示例**: 解析`1 + 2 * 3`

```
parse_expression(NONE)
  advance() → 1
  prefix: LiteralExpr(1)
  left = LiteralExpr(1)

  peek() = +, precedence(+) = TERM > NONE
    advance() → +
    infix(+): parse_expression(TERM)
      advance() → 2
      prefix: LiteralExpr(2)
      left = LiteralExpr(2)

      peek() = *, precedence(*) = FACTOR > TERM
        advance() → *
        infix(*): parse_expression(FACTOR)
          advance() → 3
          prefix: LiteralExpr(3)
          left = LiteralExpr(3)

          peek() = EOF, precedence(EOF) = NONE < FACTOR
          return LiteralExpr(3)
        left = BinaryExpr(2, *, 3)

        peek() = EOF, precedence(EOF) = NONE < TERM
        return BinaryExpr(2, *, 3)
    left = BinaryExpr(1, +, BinaryExpr(2, *, 3))

    peek() = EOF, precedence(EOF) = NONE < NONE
    return BinaryExpr(1, +, BinaryExpr(2, *, 3))
```

**结果 AST**:

```
BinaryExpr(+)
  ├─ LiteralExpr(1)
  └─ BinaryExpr(*)
      ├─ LiteralExpr(2)
      └─ LiteralExpr(3)
```

正确表示`1 + (2 * 3)`

## 递归下降解析

### 顶层解析

```cpp
shared_ptr<Program> Parser::parse() {
    auto program = make_shared<Program>();
    while (!is_at_end()) {
        auto item = parse_item();
        if (item) {
            program->items.push_back(item);
        }
    }
    return program;
}
```

**逻辑**: 循环解析所有顶层项,直到 EOF

### parse_item()

```cpp
shared_ptr<Item> Parser::parse_item() {
    if (peek().type == TokenType::FN) {
        return parse_fn_declaration();
    } else if (peek().type == TokenType::STRUCT) {
        return parse_struct_declaration();
    } else if (peek().type == TokenType::CONST) {
        return parse_const_declaration();
    } else if (peek().type == TokenType::ENUM) {
        return parse_enum_declaration();
    } else if (peek().type == TokenType::MOD) {
        return parse_mod_declaration();
    } else if (peek().type == TokenType::TRAIT) {
        return parse_trait_declaration();
    } else if (peek().type == TokenType::IMPL) {
        return parse_impl_block();
    } else {
        report_error(peek(), "Expect item declaration.");
        synchronize();
        return nullptr;
    }
}
```

**策略**: 前瞻(peek)判断类型,分派给专门的解析函数

### parse_fn_declaration()

```cpp
shared_ptr<FnDecl> Parser::parse_fn_declaration() {
    consume(TokenType::FN, "Expect 'fn' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect function name.");

    consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
    vector<shared_ptr<FnParam>> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            auto pattern = parse_pattern();
            consume(TokenType::COLON, "Expect ':' after parameter pattern.");
            auto type = parse_type();
            params.push_back(make_shared<FnParam>(pattern, type));
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");

    optional<shared_ptr<TypeNode>> return_type;
    if (match({TokenType::ARROW})) {
        return_type = parse_type();
    }

    optional<shared_ptr<BlockStmt>> body;
    if (check(TokenType::LEFT_BRACE)) {
        body = parse_block_statement();
    } else {
        consume(TokenType::SEMICOLON, "Expect ';' after function signature.");
    }

    return make_shared<FnDecl>(name, params, return_type, body);
}
```

**语法**:

```rust
fn name(param1: Type1, param2: Type2) -> ReturnType {
    body
}
```

**可选部分**:

- 返回类型: 无`->`则返回 Unit
- 函数体: 无`{}`则为声明(trait 方法)

### parse_struct_declaration()

```cpp
shared_ptr<StructDecl> Parser::parse_struct_declaration() {
    consume(TokenType::STRUCT, "Expect 'struct' keyword.");
    Token name = consume(TokenType::IDENTIFIER, "Expect struct name.");

    if (match({TokenType::SEMICOLON})) {
        // Unit struct: struct Marker;
        return make_shared<StructDecl>(name);
    } else if (match({TokenType::LEFT_PAREN})) {
        // Tuple struct: struct Color(u8, u8, u8);
        vector<shared_ptr<TypeNode>> tuple_fields;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                tuple_fields.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after tuple fields.");
        consume(TokenType::SEMICOLON, "Expect ';' after tuple struct.");
        return make_shared<StructDecl>(name, tuple_fields);
    } else {
        // Normal struct: struct Point { x: i32, y: i32 }
        consume(TokenType::LEFT_BRACE, "Expect '{' after struct name.");
        vector<shared_ptr<Field>> fields;
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            Token field_name = consume(TokenType::IDENTIFIER, "Expect field name.");
            consume(TokenType::COLON, "Expect ':' after field name.");
            auto field_type = parse_type();
            fields.push_back(make_shared<Field>(field_name, field_type));
            if (!check(TokenType::RIGHT_BRACE)) {
                consume(TokenType::COMMA, "Expect ',' after field.");
            }
        }
        consume(TokenType::RIGHT_BRACE, "Expect '}' after fields.");
        return make_shared<StructDecl>(name, fields);
    }
}
```

**三种结构体**:

1. Unit: `struct Marker;`
2. Tuple: `struct Color(u8, u8, u8);`
3. Normal: `struct Point { x: i32, y: i32 }`

### parse_if_expression()

```cpp
shared_ptr<IfExpr> Parser::parse_if_expression() {
    consume(TokenType::IF, "Expect 'if' keyword.");
    auto condition = parse_expression(Precedence::NONE);

    consume(TokenType::LEFT_BRACE, "Expect '{' after if condition.");
    auto then_branch = parse_block_expression();

    optional<shared_ptr<Expr>> else_branch;
    bool has_semicolon = false;
    if (match({TokenType::ELSE})) {
        if (match({TokenType::IF})) {
            // else if
            current_--;  // 回退if,让递归调用处理
            else_branch = parse_if_expression();
        } else {
            consume(TokenType::LEFT_BRACE, "Expect '{' after else.");
            else_branch = parse_block_expression();
        }
    }

    if (match({TokenType::SEMICOLON})) {
        has_semicolon = true;
    }

    return make_shared<IfExpr>(condition, then_branch, else_branch, has_semicolon);
}
```

**处理**:

- `if cond { ... }` → 无 else
- `if cond { ... } else { ... }` → 有 else
- `if cond { ... } else if cond2 { ... }` → 递归处理 else if

### parse_match_expression()

```cpp
shared_ptr<MatchExpr> Parser::parse_match_expression() {
    consume(TokenType::MATCH, "Expect 'match' keyword.");
    auto scrutinee = parse_expression(Precedence::NONE);

    consume(TokenType::LEFT_BRACE, "Expect '{' after match scrutinee.");
    vector<shared_ptr<MatchArm>> arms;

    while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
        auto arm = parse_match_arm();
        if (arm) arms.push_back(arm);
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after match arms.");
    return make_shared<MatchExpr>(scrutinee, arms);
}

shared_ptr<MatchArm> Parser::parse_match_arm() {
    auto pattern = parse_pattern();

    optional<shared_ptr<Expr>> guard;
    if (match({TokenType::IF})) {
        guard = parse_expression(Precedence::NONE);
    }

    consume(TokenType::FAT_ARROW, "Expect '=>' after match pattern.");
    auto body = parse_expression(Precedence::NONE);

    match({TokenType::COMMA});  // 可选的逗号

    return make_shared<MatchArm>(pattern, guard, body);
}
```

**语法**:

```rust
match value {
    pattern1 => expr1,
    pattern2 if guard => expr2,
    _ => default,
}
```

### parse_type()

```cpp
shared_ptr<TypeNode> Parser::parse_type() {
    // Never type
    if (match({TokenType::BANG})) {
        return make_shared<TypeNameNode>(previous());
    }

    // Self type
    if (match({TokenType::SELF_TYPE})) {
        return make_shared<SelfTypeNode>();
    }

    // Raw pointer: *const T, *mut T
    if (match({TokenType::STAR})) {
        bool is_mutable = match({TokenType::MUT});
        if (!is_mutable) {
            consume(TokenType::CONST, "Expect 'const' or 'mut' after '*'.");
        }
        auto pointee_type = parse_type();
        return make_shared<RawPointerTypeNode>(is_mutable, pointee_type);
    }

    // Reference: &T, &mut T
    if (match({TokenType::AMPERSAND})) {
        bool is_mutable = match({TokenType::MUT});
        auto referenced_type = parse_type();
        return make_shared<ReferenceTypeNode>(is_mutable, referenced_type);
    }

    // Array or slice: [T; N], [T]
    if (match({TokenType::LEFT_BRACKET})) {
        auto element_type = parse_type();
        if (match({TokenType::SEMICOLON})) {
            auto size = parse_expression(Precedence::NONE);
            consume(TokenType::RIGHT_BRACKET, "Expect ']'.");
            return make_shared<ArrayTypeNode>(element_type, size);
        } else {
            consume(TokenType::RIGHT_BRACKET, "Expect ']'.");
            return make_shared<SliceTypeNode>(element_type);
        }
    }

    // Unit or tuple: (), (T1, T2)
    if (match({TokenType::LEFT_PAREN})) {
        if (match({TokenType::RIGHT_PAREN})) {
            return make_shared<UnitTypeNode>();
        } else {
            vector<shared_ptr<TypeNode>> elements;
            do {
                elements.push_back(parse_type());
            } while (match({TokenType::COMMA}));
            consume(TokenType::RIGHT_PAREN, "Expect ')'.");
            return make_shared<TupleTypeNode>(elements);
        }
    }

    // Path type: MyType, Vec<T>, std::vec::Vec
    auto path = parse_path_expression();
    if (match({TokenType::LESS})) {
        // Generic: Type<Arg1, Arg2>
        vector<shared_ptr<TypeNode>> args;
        if (!check(TokenType::GREATER)) {
            do {
                args.push_back(parse_type());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::GREATER, "Expect '>'.");
        return make_shared<PathTypeNode>(path, args);
    } else {
        return make_shared<PathTypeNode>(path);
    }
}
```

**覆盖的类型**:

- 基本类型: `i32`, `bool`
- 引用: `&T`, `&mut T`
- 指针: `*const T`, `*mut T`
- 数组: `[T; N]`
- 切片: `[T]`
- 元组: `(T1, T2)`
- 泛型: `Vec<T>`
- 路径: `std::vec::Vec`

### parse_pattern()

```cpp
shared_ptr<Pattern> Parser::parse_pattern() {
    // Reference pattern: &pat, &mut pat
    if (match({TokenType::AMPERSAND})) {
        bool is_mutable = match({TokenType::MUT});
        auto pattern = parse_pattern();
        return make_shared<ReferencePattern>(is_mutable, pattern);
    }

    // Tuple pattern: (p1, p2, ...)
    if (match({TokenType::LEFT_PAREN})) {
        vector<shared_ptr<Pattern>> elements;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                elements.push_back(parse_pattern());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')'.");
        return make_shared<TuplePattern>(elements);
    }

    // Slice pattern: [p1, p2, ..]
    if (match({TokenType::LEFT_BRACKET})) {
        vector<shared_ptr<Pattern>> elements;
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                if (peek().type == TokenType::DOT_DOT) {
                    advance();
                    elements.push_back(make_shared<RestPattern>());
                    match({TokenType::COMMA});
                    break;
                }
                elements.push_back(parse_pattern());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_BRACKET, "Expect ']'.");
        return make_shared<SlicePattern>(elements);
    }

    bool is_mutable = match({TokenType::MUT});

    // Identifier or wildcard: x, mut y, _
    if (match({TokenType::IDENTIFIER})) {
        if (previous().lexeme == "_") {
            return make_shared<WildcardPattern>();
        }
        // Struct pattern: Point { x, y }
        if (peek().type == TokenType::LEFT_BRACE) {
            return parse_struct_pattern_body(make_shared<VariableExpr>(previous()));
        } else {
            return make_shared<IdentifierPattern>(previous(), is_mutable);
        }
    }

    // Literal pattern: 42, "hello", true
    if (match({TokenType::NUMBER, TokenType::STRING, TokenType::TRUE, TokenType::FALSE})) {
        return make_shared<LiteralPattern>(previous());
    }

    report_error(peek(), "Expected a pattern.");
    return nullptr;
}
```

## 工具函数

### Token 导航

```cpp
bool is_at_end() {
    return peek().type == TokenType::END_OF_FILE;
}

const Token &peek() {
    return tokens_[current_];
}

const Token &previous() {
    return tokens_[current_ - 1];
}

const Token &advance() {
    if (!is_at_end()) current_++;
    return previous();
}
```

### Token 匹配

```cpp
bool check(TokenType type) {
    if (is_at_end()) return false;
    return peek().type == type;
}

bool match(const vector<TokenType> &types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const Token &consume(TokenType type, const string &error_message) {
    if (check(type)) return advance();
    report_error(peek(), error_message);
    return peek();
}
```

### 错误处理

```cpp
void report_error(const Token &token, const string &message) {
    error_reporter_.report_error(message, token.line, token.column);
}

void synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
        case TokenType::FN:
        case TokenType::STRUCT:
        case TokenType::LET:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::RETURN:
            return;
        default:
            advance();
        }
    }
}
```

**synchronize**: 错误恢复,跳到下一个安全点(分号或关键字)

## 总结

Parser 采用 Pratt 算法处理表达式,递归下降处理语句和声明,结构清晰,易于扩展。通过注册机制和优先级表,优雅地实现了 Rust 复杂的运算符优先级。错误恢复机制保证即使遇到语法错误,也能继续解析后续代码,尽可能多地发现错误。这是一个教科书级别的 Parser 实现,值得深入学习。
