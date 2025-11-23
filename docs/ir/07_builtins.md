# 内置函数

内置函数实现了与外部世界交互的基础功能，如输入/输出和程序退出。

## 文件位置

`src/ir/ir_generator_builtins.cpp`

## 支持的内置函数

| 函数    | 功能              | C 等价                |
| ------- | ----------------- | --------------------- |
| `print` | 打印单个 i32 整数 | `printf("%d\n", ...)` |
| `scan`  | 读取单个 i32 整数 | `scanf("%d", ...)`    |
| `exit`  | 退出程序          | `exit(code)`          |

## print 函数

### 声明

```llvm
declare i32 @printf(i8*, ...)
@.str.print = private unnamed_addr constant [4 x i8] c"%d\0A\00"
```

### 用法

```rust
fn main() {
    let x = 42;
    print(x);
}
```

### 生成的 IR

```llvm
define i32 @main() {
entry:
    %x.addr = alloca i32
    store i32 42, i32* %x.addr

    %1 = load i32, i32* %x.addr
    %2 = call i32 (i8*, ...) @printf(
        i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.print, i32 0, i32 0),
        i32 %1
    )

    ret i32 0
}
```

### 实现

```cpp
std::string IRGenerator::generate_print_call(const CallExpr *call) {
    // 获取参数值
    std::string arg_reg = generate_expression(call->arguments[0].get());

    // 生成 printf 调用
    std::string result_reg = ir_emitter_.get_next_register();
    std::stringstream ss;
    ss << result_reg << " = call i32 (i8*, ...) @printf("
       << "i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.print, i32 0, i32 0), "
       << "i32 " << arg_reg
       << ")";

    ir_emitter_.emit_instruction(ss.str());
    return result_reg;
}
```

### 格式字符串

```llvm
@.str.print = private unnamed_addr constant [4 x i8] c"%d\0A\00"
```

- `%d`: 十进制整数
- `\0A`: 换行符 (ASCII 10)
- `\00`: 字符串终止符

**注意**: 自动添加换行，与 Rust `println!` 一致

## scan 函数

### 声明

```llvm
declare i32 @scanf(i8*, ...)
@.str.scan = private unnamed_addr constant [3 x i8] c"%d\00"
```

### 用法

```rust
fn main() {
    let x = scan();
    print(x);
}
```

### 生成的 IR

```llvm
define i32 @main() {
entry:
    %stack.0 = alloca i32      ; 临时存储读取的值

    %1 = call i32 (i8*, ...) @scanf(
        i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.scan, i32 0, i32 0),
        i32* %stack.0
    )

    %2 = load i32, i32* %stack.0  ; 加载结果

    %x.addr = alloca i32
    store i32 %2, i32* %x.addr

    ; ... print(x) ...
    ret i32 0
}
```

### 实现

```cpp
std::string IRGenerator::generate_scan_call(const CallExpr *call) {
    // 分配临时空间
    std::string temp_reg = ir_emitter_.get_next_alloca();
    ir_emitter_.emit_alloca(temp_reg, "i32", 4);

    // 生成 scanf 调用
    std::string call_reg = ir_emitter_.get_next_register();
    std::stringstream ss;
    ss << call_reg << " = call i32 (i8*, ...) @scanf("
       << "i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.scan, i32 0, i32 0), "
       << "i32* " << temp_reg
       << ")";
    ir_emitter_.emit_instruction(ss.str());

    // 加载结果
    std::string result_reg = ir_emitter_.get_next_register();
    ss.str("");
    ss << result_reg << " = load i32, i32* " << temp_reg;
    ir_emitter_.emit_instruction(ss.str());

    return result_reg;
}
```

### 格式字符串

```llvm
@.str.scan = private unnamed_addr constant [3 x i8] c"%d\00"
```

- `%d`: 读取十进制整数
- `\00`: 字符串终止符

**返回值**: 读取的 i32 值

## exit 函数

### 声明

```llvm
declare void @exit(i32) noreturn
```

### 用法

```rust
fn main() {
    if error {
        exit(1);
    }
    exit(0);
}
```

### 生成的 IR

```llvm
define i32 @main() {
entry:
    ; ... 错误检查 ...

    br i1 %error, label %then, label %else

then:
    call void @exit(i32 1)
    unreachable

else:
    call void @exit(i32 0)
    unreachable
}
```

### 实现

```cpp
std::string IRGenerator::generate_exit_call(const CallExpr *call) {
    // 获取退出码
    std::string code_reg = generate_expression(call->arguments[0].get());

    // 生成 exit 调用
    std::stringstream ss;
    ss << "call void @exit(i32 " << code_reg << ")";
    ir_emitter_.emit_instruction(ss.str());

    // exit 不返回
    ir_emitter_.emit_instruction("unreachable");

    return "";  // 无返回值
}
```

**关键**:

1. `noreturn` 属性告诉 LLVM 函数不会返回
2. 必须紧跟 `unreachable` 指令
3. 后续代码不可达

## 格式字符串管理

### 全局字符串池

```cpp
class IRGenerator {
private:
    std::map<std::string, std::string> string_constants_;

    std::string get_or_create_string_constant(
        const std::string &content
    ) {
        auto it = string_constants_.find(content);
        if (it != string_constants_.end()) {
            return it->second;
        }

        std::string name = "@.str." + std::to_string(string_constants_.size());
        string_constants_[content] = name;
        return name;
    }
};
```

### 字符串定义生成

```cpp
void IRGenerator::emit_string_constants() {
    for (const auto &[content, name] : string_constants_) {
        size_t len = content.length() + 1;  // 包含 \00

        std::stringstream ss;
        ss << name << " = private unnamed_addr constant ["
           << len << " x i8] c\"";

        // 转义特殊字符
        for (char c : content) {
            if (c == '\n') ss << "\\0A";
            else if (c == '\0') ss << "\\00";
            else ss << c;
        }
        ss << "\\00\"";

        global_section_ += ss.str() + "\n";
    }
}
```

## 可变参数函数

### printf 和 scanf 签名

```llvm
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
```

`...` 表示可变参数

### 调用约定

```llvm
%1 = call i32 (i8*, ...) @printf(
    i8* getelementptr(...),  ; 格式字符串
    i32 %value1,              ; 参数1
    i32 %value2,              ; 参数2
    ...                       ; 更多参数
)
```

**类型安全**: LLVM 信任格式字符串与参数匹配

## 类型检查

### 语义分析阶段

内置函数由语义分析器预定义：

```cpp
// 在 semantic_analyzer.cpp
void SemanticAnalyzer::register_builtins() {
    // print(i32) -> ()
    auto print_type = std::make_unique<FunctionType>(
        std::vector<std::unique_ptr<Type>>{
            std::make_unique<PrimitiveType>(TypeKind::I32)
        },
        std::make_unique<PrimitiveType>(TypeKind::UNIT)
    );
    builtin_functions_["print"] = std::move(print_type);

    // scan() -> i32
    auto scan_type = std::make_unique<FunctionType>(
        std::vector<std::unique_ptr<Type>>{},
        std::make_unique<PrimitiveType>(TypeKind::I32)
    );
    builtin_functions_["scan"] = std::move(scan_type);

    // exit(i32) -> !
    auto exit_type = std::make_unique<FunctionType>(
        std::vector<std::unique_ptr<Type>>{
            std::make_unique<PrimitiveType>(TypeKind::I32)
        },
        std::make_unique<NeverType>()
    );
    builtin_functions_["exit"] = std::move(exit_type);
}
```

### IR 生成阶段

```cpp
std::string IRGenerator::visit_call_expr(const CallExpr *call) {
    std::string func_name = call->function_name;

    if (func_name == "print") {
        return generate_print_call(call);
    } else if (func_name == "scan") {
        return generate_scan_call(call);
    } else if (func_name == "exit") {
        return generate_exit_call(call);
    } else {
        // 用户定义函数
        return generate_user_function_call(call);
    }
}
```

## 链接要求

### 编译为可执行文件

```bash
clang output.ll -o program
```

Clang 自动链接 C 标准库（libc），提供 printf/scanf/exit。

### 缺少实现的错误

如果未链接 C 标准库：

```
undefined reference to `printf'
undefined reference to `scanf'
undefined reference to `exit'
```

**解决**: 确保链接器能找到 libc

## 扩展内置函数

### 添加新的内置函数

1. **语义分析**: 注册函数签名
2. **IR 生成**: 添加生成逻辑
3. **外部声明**: 声明 C 函数

**示例: 添加 `abs(i32) -> i32`**

```cpp
// 语义分析
auto abs_type = std::make_unique<FunctionType>(...);
builtin_functions_["abs"] = std::move(abs_type);

// IR 生成
if (func_name == "abs") {
    std::string arg = generate_expression(call->arguments[0].get());
    std::string result = ir_emitter_.get_next_register();

    // 使用 LLVM 内置函数
    ir_emitter_.emit_instruction(
        result + " = call i32 @llvm.abs.i32(i32 " + arg + ", i1 false)"
    );

    return result;
}

// 外部声明
declare i32 @llvm.abs.i32(i32, i1)
```

## 限制与未来工作

### 当前限制

| 限制         | 说明                   |
| ------------ | ---------------------- |
| 单参数 print | 只能打印一个 i32       |
| 无格式化     | 不支持自定义格式字符串 |
| 固定类型     | print/scan 仅支持 i32  |
| 无错误处理   | scanf 失败未检查       |

### 可能的改进

1. **泛型 print**: 支持多种类型（bool, char, 字符串）
2. **格式化输出**: 类似 `printf` 的格式化
3. **多参数**: `print(x, y, z)`
4. **错误处理**: 检查 scanf 返回值
5. **更多内置函数**:
   - `assert(bool, &str)`
   - `panic(&str)`
   - `array_len<T>(array: &[T])`

## 测试覆盖

通过集成测试验证：

- ✅ print 输出正确
- ✅ scan 读取正确
- ✅ exit 终止程序
- ✅ 多次调用 print/scan
- ✅ 控制流中的内置函数
- ✅ 格式字符串正确生成

---

**参见**:

- [函数生成](./02_main_and_functions.md) - 用户定义函数
- [表达式生成](./03_expressions.md) - 函数调用表达式
- [IR 发射器](./09_ir_emitter.md) - 指令生成
