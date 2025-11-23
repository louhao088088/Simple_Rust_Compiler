# 类型映射

TypeMapper 负责将 Rust 类型映射到 LLVM IR 类型。

## 文件位置

`src/ir/type_mapper.h`, `src/ir/type_mapper.cpp`

## 核心功能

将语义分析阶段的 Rust 类型转换为 LLVM IR 类型字符串。

## 基础类型映射

| Rust 类型 | LLVM 类型 | 大小(32 位) | 对齐 |
| --------- | --------- | ----------- | ---- |
| `i32`     | `i32`     | 4 bytes     | 4    |
| `u32`     | `i32`     | 4 bytes     | 4    |
| `isize`   | `i32`     | 4 bytes     | 4    |
| `usize`   | `i32`     | 4 bytes     | 4    |
| `bool`    | `i1`      | 1 byte      | 1    |
| `char`    | `i32`     | 4 bytes     | 4    |
| `()`      | `void`    | 0           | 1    |

**注意**: 32 位平台假设

### 实现

```cpp
std::string TypeMapper::map_primitive(const PrimitiveType *type) {
    switch (type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
        return "i32";  // 32位平台
    case TypeKind::BOOL:
        return "i1";
    case TypeKind::CHAR:
        return "i32";  // Unicode 标量值
    default:
        return "i32";
    }
}
```

## 复合类型映射

### 1. 数组类型

```rust
[i32; 100]
```

映射为:

```llvm
[100 x i32]
```

**实现**:

```cpp
std::string TypeMapper::map_array(const ArrayType *type) {
    std::string elem_type = map(type->element_type.get());
    size_t size = type->size;
    return "[" + std::to_string(size) + " x " + elem_type + "]";
}
```

**嵌套数组**:

```rust
[[i32; 10]; 20]  →  [20 x [10 x i32]]
```

### 2. 结构体类型

```rust
struct Point {
    x: i32,
    y: i32,
}
```

映射为:

```llvm
%Point = type { i32, i32 }
```

**实现**:

```cpp
std::string TypeMapper::map_struct(const StructType *type) {
    return "%" + type->name;  // 使用类型名
}
```

**类型定义生成**:

```cpp
std::string TypeMapper::emit_struct_definition(const StructType *type) {
    std::stringstream ss;
    ss << "%" << type->name << " = type { ";

    for (size_t i = 0; i < type->field_order.size(); ++i) {
        if (i > 0) ss << ", ";
        std::string field_name = type->field_order[i];
        auto field_type = type->fields.at(field_name).get();
        ss << map(field_type);
    }

    ss << " }";
    return ss.str();
}
```

**字段对齐**: LLVM 自动处理结构体填充

### 3. 引用类型

```rust
&T      →  T*
&mut T  →  T*
```

**实现**:

```cpp
std::string TypeMapper::map_reference(const ReferenceType *type) {
    std::string inner_type = map(type->inner_type.get());
    return inner_type + "*";
}
```

**可变性**: LLVM IR 不区分可变/不可变引用，都是指针

### 4. 函数类型

```rust
fn(i32, i32) -> i32
```

映射为:

```llvm
i32 (i32, i32)*  // 函数指针类型
```

**当前限制**: 不支持函数指针

## 类型缓存

**优化**: 避免重复计算相同类型

```cpp
class TypeMapper {
private:
    std::unordered_map<const Type*, std::string> type_cache_;

public:
    std::string map(const Type *type) {
        auto it = type_cache_.find(type);
        if (it != type_cache_.end()) {
            return it->second;
        }

        std::string result = map_impl(type);
        type_cache_[type] = result;
        return result;
    }
};
```

## 类型大小计算

**注意**: TypeMapper 不负责类型大小，由 `IRGenerator::get_type_size()` 处理

但需要保证一致性：

| 类型    | map() 返回 | get_type_size() 返回 | 对齐 |
| ------- | ---------- | -------------------- | ---- |
| `i32`   | `i32`      | 4                    | 4    |
| `usize` | `i32`      | 4                    | 4    |
| 指针    | `T*`       | 4                    | 4    |

## 特殊类型处理

### Unit 类型

```rust
fn foo() { }  // 返回 ()
```

映射为 `void`

### 泛型类型

**当前不支持**: 泛型在语义分析阶段应该已被展开

### Trait 对象

**当前不支持**: 不支持 trait 和动态分发

## 已知问题与修复

### 1. 32 位 vs 64 位不一致

**问题**: TypeMapper 映射 usize 为 i32，但 get_type_size 返回 8

**修复**: 统一为 32 位平台

- `usize`/`isize` → `i32` (4 bytes)
- 指针 → 4 bytes

**影响**: comprehensive1 测试（线段树）

### 2. 结构体字段顺序

**要求**: 必须与 Rust 定义顺序一致

```rust
struct Foo {
    a: i32,  // 字段0
    b: i32,  // 字段1
}
```

**实现**: 使用 `field_order` 向量保证顺序

## 类型系统限制

| 特性     | 支持 |
| -------- | ---- |
| 基础类型 | ✅   |
| 数组     | ✅   |
| 结构体   | ✅   |
| 引用     | ✅   |
| 元组     | ❌   |
| 枚举     | ❌   |
| 泛型     | ❌   |
| Trait    | ❌   |
| 函数指针 | ❌   |
| 闭包     | ❌   |

## 测试覆盖

通过集成测试验证：

- ✅ 所有基础类型
- ✅ 多维数组
- ✅ 嵌套结构体
- ✅ 结构体数组
- ✅ 引用类型

---

**参见**:

- [辅助工具](./08_helpers.md) - 类型大小计算
- [IR 发射器](./09_ir_emitter.md)
- [复杂表达式](./06_complex_expressions.md)
