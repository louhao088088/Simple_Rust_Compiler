# IR 发射器

IREmitter 负责生成底层 LLVM IR 指令文本，管理寄存器命名和格式化。

## 文件位置

`src/ir/ir_emitter.h`, `src/ir/ir_emitter.cpp`

## 核心职责

1. **指令发射**: 生成各种 LLVM IR 指令
2. **寄存器管理**: 自动分配临时寄存器 (`%1`, `%2`, ...)
3. **标签管理**: 生成唯一的基本块标签
4. **格式化**: 保证生成的 IR 格式正确

## 核心成员

```cpp
class IREmitter {
private:
    std::stringstream ir_stream_;      // 主 IR 流
    std::stringstream alloca_buffer_;  // alloca 缓冲区
    int temp_counter_;                 // 临时寄存器计数
    int stack_counter_;                // 栈变量计数
    int label_counter_;                // 标签计数
    bool inside_function_;             // 是否在函数体内
};
```

## 关键特性

### 1. Alloca 提升

**问题**: Alloca 分散在函数各处，增加栈碎片

**解决**: 缓冲所有 alloca，在 `end_function()` 时统一发射到入口块

```cpp
std::string emit_alloca(const std::string &type) {
    std::string name = "%stack." + std::to_string(stack_counter_++);
    alloca_buffer_ << "  " << name << " = alloca " << type << "\n";
    return name;
}

void end_function() {
    // 发射缓冲的 alloca
    ir_stream_ << alloca_buffer_.str();
    // 发射函数体
    // ...
    alloca_buffer_.clear();
}
```

**优点**:

- 栈帧布局固定
- 支持 LLVM Mem2Reg 优化
- 代码更清晰

**命名**: 使用 `%stack.N` 避免与 SSA 寄存器 `%N` 冲突

### 2. 函数缓冲

整个函数体在 `begin_function()` 和 `end_function()` 之间缓冲：

```cpp
void begin_function(...) {
    inside_function_ = true;
    function_buffer_.clear();
    alloca_buffer_.clear();
    temp_counter_ = 0;
    stack_counter_ = 0;
    // 发射函数签名
}

void end_function() {
    // 1. 发射 alloca（提升到入口块）
    ir_stream_ << alloca_buffer_.str();
    // 2. 发射函数体
    ir_stream_ << function_buffer_.str();
    // 3. 发射结束括号
    ir_stream_ << "}\n\n";
    inside_function_ = false;
}
```

**原因**: 需要在入口块统一发射所有 alloca

## 指令发射方法

### 算术指令

```cpp
std::string emit_add(const std::string &type,
                     const std::string &lhs, const std::string &rhs);
std::string emit_sub(...);
std::string emit_mul(...);
std::string emit_sdiv(...);  // 有符号除法
std::string emit_udiv(...);  // 无符号除法
std::string emit_srem(...);  // 有符号取模
std::string emit_urem(...);  // 无符号取模
```

**示例**:

```cpp
std::string result = emitter.emit_add("i32", "%1", "%2");
// 生成: %3 = add i32 %1, %2
// 返回: "%3"
```

### 比较指令

```cpp
std::string emit_icmp(const std::string &predicate,
                      const std::string &type,
                      const std::string &lhs, const std::string &rhs);
```

**Predicates**: `eq`, `ne`, `slt`, `sle`, `sgt`, `sge`, `ult`, `ule`, `ugt`, `uge`

**示例**:

```cpp
std::string cmp = emitter.emit_icmp("slt", "i32", "%x", "%y");
// 生成: %1 = icmp slt i32 %x, %y
```

### 内存操作

```cpp
std::string emit_alloca(const std::string &type);
std::string emit_load(const std::string &type, const std::string &ptr);
void emit_store(const std::string &type,
                const std::string &value, const std::string &ptr);

std::string emit_getelementptr_inbounds(
    const std::string &type, const std::string &ptr,
    const std::vector<std::string> &indices);

void emit_memcpy(const std::string &dest, const std::string &src,
                 size_t bytes, const std::string &ptr_type);
void emit_memset(const std::string &dest, int value,
                 size_t bytes, const std::string &ptr_type);
```

**GEP 示例**:

```cpp
// arr[i] -> 获取元素指针
std::string ptr = emit_getelementptr_inbounds(
    "[100 x i32]", "%arr", {"i64 0", "i64 %i"}
);
// 生成: %1 = getelementptr inbounds [100 x i32], [100 x i32]* %arr, i64 0, i64 %i
```

**Memcpy 示例**:

```cpp
emit_memcpy("%dest", "%src", 400, "[100 x i32]*");
// 生成:
//   %1 = bitcast [100 x i32]* %dest to i8*
//   %2 = bitcast [100 x i32]* %src to i8*
//   call void @llvm.memcpy.p0.p0.i64(i8* %1, i8* %2, i64 400, i1 false)
```

### 控制流

```cpp
void emit_br(const std::string &label);
void emit_cond_br(const std::string &condition,
                  const std::string &true_label,
                  const std::string &false_label);
void emit_ret(const std::string &type, const std::string &value);
void emit_ret_void();

std::string emit_phi(const std::string &type,
                     const std::vector<std::pair<std::string, std::string>> &incoming);
```

**PHI 示例**:

```cpp
std::vector<std::pair<std::string, std::string>> incoming = {
    {"10", "if.then.0"},
    {"20", "if.else.0"}
};
std::string result = emit_phi("i32", incoming);
// 生成: %1 = phi i32 [10, %if.then.0], [20, %if.else.0]
```

### 类型转换

```cpp
std::string emit_bitcast(const std::string &src_type,
                         const std::string &value,
                         const std::string &dest_type);
std::string emit_sext(...);   // 符号扩展
std::string emit_zext(...);   // 零扩展
std::string emit_trunc(...);  // 截断
```

### 函数调用

```cpp
std::string emit_call(const std::string &ret_type,
                      const std::string &func_name,
                      const std::vector<std::pair<std::string, std::string>> &args);

void emit_call_void(const std::string &func_name,
                    const std::vector<std::pair<std::string, std::string>> &args);
```

**示例**:

```cpp
std::vector<std::pair<std::string, std::string>> args = {
    {"i32", "%x"},
    {"i32*", "%y"}
};
std::string result = emit_call("i32", "@foo", args);
// 生成: %1 = call i32 @foo(i32 %x, i32* %y)
```

## 辅助方法

### 标签生成

```cpp
std::string new_label() {
    return "label" + std::to_string(label_counter_++);
}
```

### 寄存器重置

```cpp
void reset_temp_counter() {
    temp_counter_ = 0;
}
```

**用途**: 每个函数开始时重置，从 `%0` 开始编号

### 输出获取

```cpp
std::string get_ir() const {
    return ir_stream_.str();
}
```

## 格式化规则

### 缩进

- 函数级声明: 不缩进
- 基本块标签: 不缩进
- 指令: 2 空格缩进

```llvm
define i32 @foo() {
bb.entry:
  %1 = add i32 1, 2
  ret i32 %1
}
```

### 命名约定

| 类型       | 格式       | 示例                        |
| ---------- | ---------- | --------------------------- |
| 临时寄存器 | `%N`       | `%0`, `%1`, `%2`            |
| 栈变量     | `%stack.N` | `%stack.0`, `%stack.1`      |
| 函数参数   | `%name`    | `%x`, `%arr`                |
| 全局变量   | `@name`    | `@MAX`, `@.str.format`      |
| 类型       | `%Name`    | `%Point`, `%Node`           |
| 标签       | `name.N`   | `if.then.0`, `while.cond.1` |

### 注释

生成的 IR 不包含注释（可添加作为调试辅助）。

## 性能考虑

### 字符串拼接

使用 `std::stringstream` 避免大量字符串拼接开销。

### 缓冲策略

函数体缓冲避免频繁写入输出流。

## 测试

IREmitter 通过集成测试覆盖，没有独立单元测试。

**验证方法**:

1. 生成 IR
2. 运行 `llc` 编译到汇编
3. 如果 llc 成功，IR 格式正确

---

**参见**:

- [主类设计](./01_ir_generator.md)
- [类型映射](./10_type_mapper.md)
