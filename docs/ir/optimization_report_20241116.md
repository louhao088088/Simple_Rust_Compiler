# IR 优化报告 - 2024-11-16

## 优化概述

本次优化针对 IR 生成器的性能瓶颈进行了多项改进，主要关注编译时性能和生成代码质量。

## 实施的优化

### 1. GEP 指令缓存

**位置**: `src/ir/ir_generator_complex_exprs.cpp`

**实现**: 在同一基本块内缓存 GEP 计算结果

```cpp
std::unordered_map<std::string, std::string> gep_cache_;
```

**效果**: 避免重复的 getelementptr 计算，特别是在结构体字段频繁访问时

### 2. 类型大小缓存

**位置**: `src/ir/ir_generator_helpers.cpp`

**实现**: 缓存类型大小计算结果

```cpp
std::unordered_map<Type*, size_t> type_size_cache_;
```

**效果**:

- 编译时间从慢速降低到 ~0.03s (comprehensive14)
- 避免重复遍历结构体字段

### 3. 字段索引缓存

**位置**: `src/ir/ir_generator_complex_exprs.cpp`

**实现**: 缓存"struct_name.field_name" -> index 映射

```cpp
std::unordered_map<std::string, int> field_index_cache_;
```

**效果**: 字段访问从 O(n)降低到 O(1)

### 4. Placeholder 系统与 Alloca 提升

**位置**: `src/ir/ir_emitter.cpp`, `src/ir/ir_emitter.h`

**实现**:

- Entry block 使用占位符: `__alloca_N`, `__temp_N`
- 在`finish_entry_block()`时统一分配连续的 SSA 编号
- 智能替换避免部分匹配(如`__alloca_5`不会误匹配`__alloca_50`)

**效果**:

- 所有 entry block 的 alloca 指令在函数开头
- 生成的 IR 具有连续的 SSA 编号(%0, %1, %2...)
- 代码可读性和规范性提升

## 性能测试结果

### 编译性能

| 测试用例        | 优化前     | 优化后 | 提升 |
| --------------- | ---------- | ------ | ---- |
| comprehensive14 | 慢速(多秒) | ~0.03s | 显著 |

### 测试通过率

- **47/50** 测试通过
- 与优化前保持一致，无功能退化

### 运行时性能

| 测试用例        | 运行时间 | 状态    |
| --------------- | -------- | ------- |
| comprehensive41 | ~24s     | ✅ 通过 |
| comprehensive12 | ~27s     | ✅ 通过 |
| comprehensive9  | ~15s     | ✅ 通过 |
| comprehensive14 | >30s     | ❌ 超时 |
| comprehensive15 | >30s     | ❌ 超时 |
| comprehensive21 | >30s     | ❌ 超时 |

## 代码质量改进

### 优化前的 IR 片段:

```llvm
while.body.39:
  %50 = alloca i32    ; 在循环体内重复分配
  %73 = alloca i32    ; 在循环体内重复分配
  ; 编号不连续: %50, %73...
```

### 优化后的 IR 片段:

```llvm
bb.entry:
  %0 = alloca i32     ; 所有alloca在entry block
  %1 = alloca i32
  %2 = alloca %SortingAnalyzer
  %3 = alloca %DataProcessor
  ; 编号连续: %0, %1, %2, %3...

while.body.39:
  %18 = alloca i32    ; 循环内alloca仍存在(待优化)
  %23 = alloca i32
```

## 已知限制

### 1. 循环内 Alloca

**问题**: 循环体内声明的变量(如`let mut j`)仍然在循环内分配

```rust
while i < n {
    let mut j = 0;  // 每次循环都重新alloca
    // ...
}
```

**影响**: 重复的 alloca 操作影响性能

**解决方案**: 需要更高级的分析:

- 方案 A: 实现完整的 mem2reg 优化 pass
- 方案 B: 在语义分析阶段识别循环内变量，提前提升 alloca
- 方案 C: 集成 LLVM 优化管道(opt 工具)

### 2. 参数不必要的 Alloca

**问题**: 基本类型参数被不必要地 alloca 和 store

```llvm
define i32 @function(i32 %param) {
  %0 = alloca i32        ; 不必要
  store i32 %param, i32* %0  ; 不必要
  %1 = load i32, i32* %0     ; 可以直接用%param
}
```

**解决方案**: 需要活性分析，判断参数是否真正需要可变存储

### 3. 仍然超时的测试

comp14, comp15, comp21 仍然超过 30 秒超时限制，可能原因:

- 算法复杂度高(O(n³)或更高)
- 大量循环内 alloca 开销
- 需要 LLVM 的 mem2reg 优化

## 技术亮点

### Placeholder 替换的正确性

使用边界检查避免误替换:

```cpp
bool valid_start = (pos == 0 || (!std::isalnum(result[pos-1]) && result[pos-1] != '_'));
bool valid_end = (pos + placeholder.length() >= result.length() ||
                 (!std::isalnum(result[pos + placeholder.length()]) &&
                  result[pos + placeholder.length()] != '_'));
```

这确保了`__alloca_5`不会误匹配`__alloca_50`。

## 下一步建议

### 短期优化

1. 循环内变量提升: 分析变量作用域，提升循环内 alloca
2. 参数优化: 只在需要可变引用时才 alloca 参数
3. 死代码消除: 移除未使用的 alloca

### 长期优化

1. 集成 LLVM 优化管道: 使用`opt -mem2reg -O2`
2. SSA 构造优化: 减少不必要的 load/store 对
3. 寄存器分配改进: 更好的临时变量管理

## 总结

本次优化主要改进了编译时性能和代码质量:

- ✅ 编译速度显著提升
- ✅ 生成 IR 更加规范(连续编号、entry block alloca)
- ✅ 保持测试通过率(47/50)
- ⚠️ 运行时性能需要更高级的优化(mem2reg)

当前实现在不破坏功能的前提下，为后续更深层次的优化奠定了基础。
