# 辅助工具

辅助工具模块提供类型大小计算、常量求值、零初始化检测等实用函数。

## 文件位置

`src/ir/ir_generator_helpers.cpp`

## 类型大小计算

### get_type_size()

计算类型在内存中的字节大小。

#### 基础类型

| 类型             | 大小    | 说明           |
| ---------------- | ------- | -------------- |
| `i32`, `u32`     | 4 bytes | 32 位整数      |
| `isize`, `usize` | 4 bytes | 32 位平台      |
| `char`           | 4 bytes | Unicode 标量值 |
| `bool`           | 1 byte  | 布尔值         |
| `()`             | 0 bytes | Unit 类型      |
| 指针/引用        | 4 bytes | 32 位平台      |

**实现**:

```cpp
size_t IRGenerator::get_type_size(const Type *type) {
    if (auto prim = dynamic_cast<const PrimitiveType*>(type)) {
        switch (prim->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::ISIZE:
        case TypeKind::USIZE:
        case TypeKind::CHAR:
            return 4;
        case TypeKind::BOOL:
            return 1;
        case TypeKind::UNIT:
            return 0;
        default:
            return 4;
        }
    }
    // ... 其他类型 ...
}
```

#### 数组类型

```
size = element_size × array_length
```

**示例**:

```rust
[i32; 100]  →  4 × 100 = 400 bytes
[[i32; 10]; 20]  →  (4 × 10) × 20 = 800 bytes
```

**实现**:

```cpp
if (auto arr = dynamic_cast<const ArrayType*>(type)) {
    size_t elem_size = get_type_size(arr->element_type.get());
    return elem_size * arr->size;
}
```

#### 结构体类型

```
size = Σ(field_size) + padding
```

**示例**:

```rust
struct Point {
    x: i32,  // 4 bytes
    y: i32,  // 4 bytes
}
// 总大小: 8 bytes
```

**实现**:

```cpp
if (auto st = dynamic_cast<const StructType*>(type)) {
    size_t total = 0;
    for (const auto &field_name : st->field_order) {
        auto field_type = st->fields.at(field_name).get();
        total += get_type_size(field_type);
    }
    return total;
}
```

**注意**: 当前不计算填充，假设紧密排列

#### 引用类型

```rust
&T      →  4 bytes (指针)
&mut T  →  4 bytes (指针)
```

**实现**:

```cpp
if (auto ref = dynamic_cast<const ReferenceType*>(type)) {
    return 4;  // 32位指针
}
```

## 类型对齐计算

### get_type_alignment()

计算类型的对齐要求。

#### 对齐规则

| 类型                                   | 对齐         |
| -------------------------------------- | ------------ |
| `i32`, `u32`, `usize`, `isize`, `char` | 4            |
| `bool`                                 | 1            |
| 指针/引用                              | 4            |
| 数组                                   | 元素对齐     |
| 结构体                                 | 最大字段对齐 |

**实现**:

```cpp
size_t IRGenerator::get_type_alignment(const Type *type) {
    if (auto prim = dynamic_cast<const PrimitiveType*>(type)) {
        switch (prim->kind) {
        case TypeKind::I32:
        case TypeKind::U32:
        case TypeKind::ISIZE:
        case TypeKind::USIZE:
        case TypeKind::CHAR:
            return 4;
        case TypeKind::BOOL:
            return 1;
        default:
            return 4;
        }
    }

    if (auto arr = dynamic_cast<const ArrayType*>(type)) {
        return get_type_alignment(arr->element_type.get());
    }

    if (auto st = dynamic_cast<const StructType*>(type)) {
        size_t max_align = 1;
        for (const auto &field_name : st->field_order) {
            auto field_type = st->fields.at(field_name).get();
            max_align = std::max(max_align, get_type_alignment(field_type));
        }
        return max_align;
    }

    if (dynamic_cast<const ReferenceType*>(type)) {
        return 4;
    }

    return 4;  // 默认
}
```

## 常量求值

### is_constant_zero()

检查表达式是否为编译期常量 0。

**用途**: 优化 `memset` 调用

```rust
let arr = [0; 100];  // 可以优化为 memset
```

**实现**:

```cpp
bool IRGenerator::is_constant_zero(const Expr *expr) {
    if (auto lit = dynamic_cast<const IntLiteralExpr*>(expr)) {
        return lit->value == 0;
    }

    if (auto arr = dynamic_cast<const ArrayExpr*>(expr)) {
        // 检查所有元素是否为 0
        for (const auto &elem : arr->elements) {
            if (!is_constant_zero(elem.get())) {
                return false;
            }
        }
        return true;
    }

    if (auto st = dynamic_cast<const StructExpr*>(expr)) {
        // 检查所有字段是否为 0
        for (const auto &[name, value] : st->fields) {
            if (!is_constant_zero(value.get())) {
                return false;
            }
        }
        return true;
    }

    return false;
}
```

**优化示例**:

```rust
let arr = [0; 1000];
```

**未优化**:

```llvm
%arr.addr = alloca [1000 x i32]
store i32 0, [1000 x i32]* %arr.addr, i32 0, i32 0
store i32 0, [1000 x i32]* %arr.addr, i32 0, i32 1
; ... 1000 条 store 指令 ...
```

**优化后**:

```llvm
%arr.addr = alloca [1000 x i32]
%1 = bitcast [1000 x i32]* %arr.addr to i8*
call void @llvm.memset.p0i8.i32(i8* %1, i8 0, i32 4000, i1 false)
```

**节省**: 999 条指令 → 2 条指令

## 零初始化检测

### is_zero_initializer()

检查值是否为零初始化（递归检查）。

**实现**:

```cpp
bool IRGenerator::is_zero_initializer(const Expr *expr) {
    return is_constant_zero(expr);  // 当前实现相同
}
```

**用于**: 决定是否使用 `memset` 或逐元素初始化

## 结构体字段索引

### get_field_index()

获取结构体字段的索引（用于 GEP 指令）。

```rust
struct Point {
    x: i32,  // 索引 0
    y: i32,  // 索引 1
}
```

**实现**:

```cpp
int IRGenerator::get_field_index(
    const StructType *struct_type,
    const std::string &field_name
) {
    const auto &field_order = struct_type->field_order;

    for (size_t i = 0; i < field_order.size(); ++i) {
        if (field_order[i] == field_name) {
            return static_cast<int>(i);
        }
    }

    // 字段不存在（语义分析应该已检查）
    return -1;
}
```

**使用**:

```cpp
// 生成 p.x
int field_idx = get_field_index(point_type, "x");  // 0
std::string gep_result = ir_emitter_.get_next_register();
ir_emitter_.emit_instruction(
    gep_result + " = getelementptr %Point, %Point* " + base_ptr +
    ", i32 0, i32 " + std::to_string(field_idx)
);
```

## 类型转换检查

### needs_type_conversion()

检查是否需要类型转换（如 bool → i32）。

```rust
let x: i32 = true;  // bool 需要转换为 i32
```

**实现**:

```cpp
bool IRGenerator::needs_type_conversion(
    const Type *from_type,
    const Type *to_type
) {
    // bool → i32: zext
    if (is_bool_type(from_type) && is_i32_type(to_type)) {
        return true;
    }

    // i32 → bool: icmp ne 0
    if (is_i32_type(from_type) && is_bool_type(to_type)) {
        return true;
    }

    // 其他情况暂不需要转换
    return false;
}
```

### convert_type()

执行类型转换。

```cpp
std::string IRGenerator::convert_type(
    const std::string &value_reg,
    const Type *from_type,
    const Type *to_type
) {
    if (!needs_type_conversion(from_type, to_type)) {
        return value_reg;
    }

    std::string result_reg = ir_emitter_.get_next_register();

    if (is_bool_type(from_type) && is_i32_type(to_type)) {
        // bool → i32: zext i1 to i32
        ir_emitter_.emit_instruction(
            result_reg + " = zext i1 " + value_reg + " to i32"
        );
    } else if (is_i32_type(from_type) && is_bool_type(to_type)) {
        // i32 → bool: icmp ne 0
        ir_emitter_.emit_instruction(
            result_reg + " = icmp ne i32 " + value_reg + ", 0"
        );
    }

    return result_reg;
}
```

## 表达式类型推断

### get_expression_type()

获取表达式的类型（从 AST 节点）。

```cpp
const Type* IRGenerator::get_expression_type(const Expr *expr) {
    // 表达式的类型已由语义分析确定
    return expr->type.get();
}
```

**注意**: 依赖语义分析的类型标注

## 寄存器名生成

### generate_unique_label()

生成唯一的 LLVM 标签名。

```cpp
std::string IRGenerator::generate_unique_label(const std::string &prefix) {
    static int label_counter = 0;
    return prefix + std::to_string(label_counter++);
}
```

**用法**:

```cpp
std::string then_label = generate_unique_label("if.then");
std::string else_label = generate_unique_label("if.else");
std::string merge_label = generate_unique_label("if.merge");
```

**生成**:

```
if.then0, if.else0, if.merge0
if.then1, if.else1, if.merge1
...
```

## 内存布局计算

### calculate_struct_layout()

计算结构体字段的偏移量（考虑对齐）。

**当前实现**: 假设 LLVM 自动处理对齐

**未来**: 显式计算填充

```cpp
struct FieldLayout {
    size_t offset;
    size_t size;
};

std::vector<FieldLayout> calculate_struct_layout(const StructType *type) {
    std::vector<FieldLayout> layout;
    size_t current_offset = 0;

    for (const auto &field_name : type->field_order) {
        auto field_type = type->fields.at(field_name).get();
        size_t field_size = get_type_size(field_type);
        size_t field_align = get_type_alignment(field_type);

        // 对齐当前偏移
        current_offset = (current_offset + field_align - 1) / field_align * field_align;

        layout.push_back({current_offset, field_size});
        current_offset += field_size;
    }

    return layout;
}
```

## 常量折叠

### evaluate_constant_expression()

编译期常量求值（有限支持）。

```rust
const SIZE: usize = 10 * 10;  // 编译期计算为 100
```

**当前限制**: 仅支持字面量，不支持算术表达式

**未来实现**:

```cpp
std::optional<int64_t> evaluate_constant_expression(const Expr *expr) {
    if (auto lit = dynamic_cast<const IntLiteralExpr*>(expr)) {
        return lit->value;
    }

    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        auto left = evaluate_constant_expression(bin->left.get());
        auto right = evaluate_constant_expression(bin->right.get());

        if (!left || !right) return std::nullopt;

        switch (bin->op) {
        case BinaryOp::ADD: return *left + *right;
        case BinaryOp::SUB: return *left - *right;
        case BinaryOp::MUL: return *left * *right;
        case BinaryOp::DIV:
            if (*right == 0) return std::nullopt;
            return *left / *right;
        default: return std::nullopt;
        }
    }

    return std::nullopt;
}
```

## 错误处理辅助

### format_error_message()

格式化错误消息（包含位置信息）。

```cpp
std::string IRGenerator::format_error_message(
    const std::string &message,
    const ASTNode *node
) {
    std::stringstream ss;
    ss << "IR Generation Error at line " << node->line
       << ", column " << node->column << ": " << message;
    return ss.str();
}
```

**用法**:

```cpp
if (!validate_something()) {
    std::string error = format_error_message(
        "Invalid array size",
        array_expr
    );
    error_handler_.report(error);
}
```

## 32 位 vs 64 位平台

### 平台特定常量

```cpp
namespace PlatformConfig {
    constexpr size_t POINTER_SIZE = 4;    // 32位平台
    constexpr size_t POINTER_ALIGN = 4;
    constexpr size_t ISIZE_SIZE = 4;
    constexpr size_t USIZE_SIZE = 4;
}
```

**使用**:

```cpp
size_t get_type_size(const Type *type) {
    if (is_pointer_type(type)) {
        return PlatformConfig::POINTER_SIZE;
    }
    // ...
}
```

**迁移到 64 位**: 只需修改常量

```cpp
constexpr size_t POINTER_SIZE = 8;    // 64位平台
constexpr size_t ISIZE_SIZE = 8;
```

## 性能优化

### 类型大小缓存

```cpp
class IRGenerator {
private:
    std::unordered_map<const Type*, size_t> type_size_cache_;

public:
    size_t get_type_size(const Type *type) {
        auto it = type_size_cache_.find(type);
        if (it != type_size_cache_.end()) {
            return it->second;
        }

        size_t size = calculate_type_size(type);
        type_size_cache_[type] = size;
        return size;
    }
};
```

**收益**: 避免重复计算复杂结构体大小

## 测试覆盖

- ✅ 所有基础类型大小正确
- ✅ 多维数组大小计算
- ✅ 结构体大小计算
- ✅ 32 位平台一致性（usize=4, 指针=4）
- ✅ 零初始化检测
- ✅ 字段索引查找

## 已知问题

### 结构体填充

**当前**: 不计算填充，依赖 LLVM 自动对齐

**问题**: 手动计算大小时可能不准确

**示例**:

```rust
struct Mixed {
    a: bool,   // 1 byte
    b: i32,    // 4 bytes，对齐到4字节边界
}
```

**实际布局**:

```
offset 0: a (1 byte)
offset 1-3: padding (3 bytes)
offset 4: b (4 bytes)
总大小: 8 bytes
```

**当前计算**: 1 + 4 = 5 bytes (错误)

**解决**: 实现 `calculate_struct_layout()` 考虑对齐

---

**参见**:

- [类型映射](./10_type_mapper.md) - 类型字符串生成
- [复杂表达式](./06_complex_expressions.md) - 使用类型大小
- [IR 发射器](./09_ir_emitter.md) - Alloca 和内存操作
