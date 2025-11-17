# TypeMapper 实现设计分析

## 任务概述

实现 TypeMapper 类，将 Rust 类型系统映射到 LLVM IR 类型字符串表示。

## 现有类型系统分析

### 语义分析模块的类型（semantic.h）

当前项目中已定义的类型：

```cpp
enum class TypeKind {
    // 基础整数类型
    I32, U32, ISIZE, USIZE, ANY_INTEGER,

    // 字符串类型
    STR, STRING, RSTRING, CSTRING, RCSTRING,

    // 其他基础类型
    CHAR, BOOL,

    // 复合类型
    ARRAY, STRUCT, UNIT, FUNCTION, REFERENCE, ENUM, RAW_POINTER,

    // 特殊类型
    NEVER, UNKNOWN,
};

// 类型结构体
- PrimitiveType
- ArrayType (element_type, size)
- StructType (name, fields map)
- UnitType
- NeverType
- FunctionType (return_type, param_types)
- ReferenceType (referenced_type, is_mutable)
- RawPointerType (pointee_type, is_mutable)
- EnumType (name, variants)
```

## 🔴 发现的问题

### 问题 1: 字符串类型映射不清晰

**问题描述**:
Rust 中有多种字符串类型 (STR, STRING, RSTRING, CSTRING, RCSTRING)，但 LLVM IR 中没有直接对应的字符串类型。

**疑问**:

1. `&str` 应该映射为什么？

   - 选项 A: `i8*` (C 风格字符串指针)
   - 选项 B: `{ i8*, i64 }` (指针+长度的胖指针)
   - 选项 C: `{ i8*, i64 }*` (胖指针的指针)

2. `String` 应该映射为什么？

   - 选项 A: `i8*` (简化处理)
   - 选项 B: `{ i8*, i64, i64 }` (指针+长度+容量)
   - 选项 C: `%String` (自定义结构体)

3. RSTRING, CSTRING, RCSTRING 是什么？
   - 项目特定的字符串类型？
   - 需要确认其内存布局

**建议方案**:

- 暂时将所有字符串类型都映射为 `i8*`（简单指针）
- 后续根据实际需求调整为胖指针结构

### 问题 2: 缺少 I8/I16/I64/U8/U16/U64 类型

**问题描述**:
文档中 TypeMapper 实现提到了 I8, I16, I64, U8, U16 等类型，但 `TypeKind` 枚举中只定义了：

- I32, U32, ISIZE, USIZE

**疑问**:
是否需要支持这些类型？如果需要，应该：

1. 在 TypeKind 中添加这些枚举值
2. 或者通过某种方式扩展现有类型系统

**建议方案**:

- 先按现有的 TypeKind 实现
- 如果遇到其他整数类型，返回默认映射（如 i32）
- 记录 TODO 供后续扩展

### 问题 3: EnumType 映射复杂

**问题描述**:
Rust 的 enum 可以是：

- 简单枚举 (C 风格): `enum Color { Red, Green, Blue }`
- 带数据的枚举: `enum Option<T> { Some(T), None }`

**疑问**:

1. 简单枚举应该映射为：

   - `i32` (C 风格整数)
   - `i8` (节省空间)

2. 带数据的枚举应该映射为：
   - Tagged union 结构
   - 需要判别式(discriminant)字段

**建议方案**:

- 第一阶段：简单枚举映射为 `i32`
- 复杂枚举暂时返回 `i32`，添加 TODO 注释
- 后续实现完整的 tagged union 支持

### 问题 4: NEVER 类型的映射

**问题描述**:
Rust 的 `!` (never) 类型表示永不返回的函数。

**疑问**:
LLVM 中应该如何表示？

- 选项 A: `void` (但 void 可以返回)
- 选项 B: 不映射，因为 never 类型的表达式实际不会产生值
- 选项 C: 保持为某个占位符

**建议方案**:

- 映射为 `void`
- 在代码生成时特殊处理 never 类型的表达式（不需要生成实际的返回值代码）

### 问题 5: StructType 字段获取问题

**问题描述**:
文档中 TypeMapper 实现访问 `type->fields` 并假设它是一个 vector:

```cpp
for (size_t i = 0; i < type->fields.size(); ++i) {
    ss << map(type->fields[i].type.get());
}
```

但 semantic.h 中 StructType 的定义是：

```cpp
struct StructType : public Type {
    std::string name;
    std::map<std::string, std::shared_ptr<Type>> fields;  // ⚠️ 是map不是vector！
    std::weak_ptr<Symbol> symbol;
};
```

**问题**:

- fields 是 `std::map<std::string, std::shared_ptr<Type>>`，不是 vector
- 直接迭代 map 可能导致字段顺序不确定（map 按 key 排序）
- 需要确保字段顺序与结构体定义中的顺序一致

**建议方案**:

```cpp
std::string TypeMapper::declare_struct_type(const StructType* type) {
    if (declared_structs_[type->name]) {
        return "";
    }
    declared_structs_[type->name] = true;

    std::stringstream ss;
    ss << "%" << type->name << " = type { ";

    // ⚠️ 需要从Symbol中获取正确的字段顺序
    // 选项1: 遍历map（但顺序可能不对）
    size_t i = 0;
    for (const auto& [field_name, field_type] : type->fields) {
        if (i > 0) ss << ", ";
        ss << map(field_type.get());
        i++;
    }

    ss << " }\n";
    return ss.str();
}
```

**更好的方案**:
需要从 StructDecl 的 AST 节点中获取字段的原始声明顺序，或者在 StructType 中维护一个有序的字段列表。

### 问题 6: 文档与实际不匹配 - ValueManager 使用了 LLVM 类型

**问题描述**:
文档第一阶段指南中的 ValueManager 实现使用了 LLVM API：

```cpp
struct VariableInfo {
    llvm::Value* alloca_inst;   // ❌ 使用了llvm::Value
    llvm::Type* type;           // ❌ 使用了llvm::Type
    bool is_mutable;
};
```

但我们的核心理念是"不使用 LLVM C++ API"！

**疑问**:
文档是否有误？还是第一阶段就允许使用 LLVM API？

**建议方案**:
修改为纯字符串实现：

```cpp
struct VariableInfo {
    std::string alloca_name;     // IR变量名，如"%x.addr"
    std::string type_str;        // IR类型字符串，如"i32"
    bool is_mutable;
};
```

## 实现策略

### TypeMapper 核心接口

```cpp
class TypeMapper {
public:
    TypeMapper(BuiltinTypes& builtin_types);

    // 主转换接口
    std::string map(const Type* rust_type);

    // 特定类型转换
    std::string map_primitive(const PrimitiveType* type);
    std::string map_array(const ArrayType* type);
    std::string map_struct(const StructType* type);
    std::string map_function(const FunctionType* type);
    std::string map_reference(const ReferenceType* type);
    std::string map_raw_pointer(const RawPointerType* type);
    std::string map_enum(const EnumType* type);

    // 获取类型的零值
    std::string get_zero_value(const Type* type);

    // 注册结构体类型定义
    std::string declare_struct_type(const StructType* type);

private:
    BuiltinTypes& builtin_types_;
    std::unordered_map<const Type*, std::string> type_cache_;
    std::unordered_map<std::string, bool> declared_structs_;
};
```

### 类型映射表（初步设计）

| Rust Type         | LLVM IR Type  | 注释                        |
| ----------------- | ------------- | --------------------------- |
| `i32`             | `i32`         | 32 位有符号整数             |
| `u32`             | `i32`         | LLVM 不区分有无符号         |
| `i64`/`isize`     | `i64`         | 64 位整数（假设 64 位平台） |
| `u64`/`usize`     | `i64`         | 64 位无符号                 |
| `bool`            | `i1`          | 1 位布尔值                  |
| `char`            | `i32`         | Unicode 标量值              |
| `()` (unit)       | `void`        | 空类型                      |
| `!` (never)       | `void`        | 永不返回                    |
| `[T; N]`          | `[N x T]`     | 固定大小数组                |
| `&T`              | `T*`          | 不可变引用 → 指针           |
| `&mut T`          | `T*`          | 可变引用 → 指针             |
| `*const T`        | `T*`          | 原始指针                    |
| `*mut T`          | `T*`          | 可变原始指针                |
| `struct Foo`      | `%Foo`        | 命名结构体类型              |
| `enum E`          | `i32`         | 简单枚举（暂时）            |
| `fn(T1, T2) -> R` | `R (T1, T2)*` | 函数指针                    |
| `str`             | `i8*`         | 字符串切片（暂时）          |
| `String`          | `i8*`         | 字符串（暂时）              |

### 实现优先级

1. **高优先级** (第一阶段必须):

   - ✅ 基础整数类型 (i32, u32, i64, u64, isize, usize)
   - ✅ bool, char, unit
   - ✅ 数组类型
   - ✅ 引用类型（映射为指针）
   - ✅ 结构体类型（名称引用）

2. **中优先级** (第二阶段):

   - 🔶 结构体类型声明（需要正确的字段顺序）
   - 🔶 函数指针类型
   - 🔶 原始指针类型

3. **低优先级** (后续完善):
   - 🔻 复杂枚举类型（tagged union）
   - 🔻 字符串类型的完整实现（胖指针）
   - 🔻 泛型类型的单态化

## 待解决问题清单

1. **字符串类型的最终映射方案** - 需要确认
2. **结构体字段顺序问题** - 需要从 AST 获取或修改 StructType 定义
3. **是否需要支持 I8/I16 等类型** - 需要确认
4. **EnumType 的完整实现** - 第一阶段暂时简化
5. **ValueManager 的 LLVM API 依赖** - 需要修正文档或实现

## 下一步行动

1. **先实现基础版本的 TypeMapper**

   - 只处理高优先级类型
   - 对于复杂情况返回占位符+TODO 注释

2. **编写测试用例验证映射正确性**

3. **根据实际使用情况调整设计**

4. **与项目负责人确认上述问题的解决方案**
