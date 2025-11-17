# IR 生成调试总结 - Phase 2H

## 当前状态

**测试通过率: 31/50 (62%)**

## 已完成的修复

### 1. void 类型 PHI 节点问题 (comp26/27)

**问题**: IfExpr 为 unit 类型表达式生成了`phi void`节点
**解决方案**:

- 在生成 PHI 节点前检查类型是否为 unit
- 只为非空 result 添加到 PHI 的 incoming 列表
- 代码位置: `ir_generator_control_flow.cpp` 第 119-145 行

### 2. PHI 节点前驱块标签错误 (comp27)

**问题**: PHI 节点使用错误的前驱块标签（使用初始块而非最终块）
**根本原因**: `WhileExpr`和`LoopExpr`使用`emitter_.begin_basic_block()`而非`begin_block()`
**解决方案**:

- 统一使用`begin_block()`来同步更新`current_block_label_`
- 修改位置: `ir_generator_control_flow.cpp` WhileExpr 和 LoopExpr

### 3. UnaryExpr 解引用聚合类型问题 (comp7)

**问题**: `*graph`解引用数组时加载了整个数组值
**解决方案**:

- 聚合类型解引用返回指针而非加载值
- 代码: `ir_generator_expressions.cpp` 第 207-217 行

### 4. 核心聚合类型处理机制

所有已实现和验证的处理：

- ✅ VariableExpr: 聚合类型返回指针
- ✅ IndexExpr: 正确的`generating_lvalue_`管理
- ✅ FieldAccessExpr: 聚合字段返回指针
- ✅ AssignmentExpr: 右侧聚合值加载
- ✅ ReturnStmt: 聚合返回值加载
- ✅ IfExpr PHI: 使用聚合指针类型
- ✅ 数组索引类型转换: i32→i64 (32 位平台)

## 剩余失败测试分析 (19 个)

### A. 逻辑运算符短路求值问题 (2 个: comp13, comp15)

**错误**: `br i1 ,` - 条件表达式为空
**根本原因**:

- `||`和`&&`运算符的左右操作数都为空
- 当前实现使用简单的`and`/`or`指令，无法正确处理复杂条件
- 需要实现短路求值（control flow based）

**示例代码** (comp13 第 316 行):

```rust
if (self.evaluate_polynomial(next_x) == 0
    || (if (...) { ... } else { ... }) <= 5)
```

**需要的实现**:

```llvm
; a || b (短路求值)
  计算a
  br i1 %a, label %or.true, label %or.rhs
or.rhs:
  计算b
  br label %or.end
or.true:
  br label %or.end
or.end:
  %result = phi i1 [true, %or.true], [%b, %or.rhs]
```

**优先级**: 高 - 影响 2 个测试

### B. 类型不匹配问题 (1 个: comp29)

**错误**: `store i32 %354, i32* %357` 但`%354`类型是 i1
**原因**: bool 值(i1)被用于需要 i32 的上下文
**优先级**: 中

### C. 双重指针问题 (3 个: comp20, comp33, comp34)

**错误**: `getelementptr`期望`%Type*`但收到`%Type**`
**示例**:

```
%10' defined with type '%Transition**' but expected '%Transition*'
%11' defined with type '%Agent**' but expected '%Agent*'
```

**可能原因**: 引用类型处理错误，多余的间接层
**优先级**: 高 - 影响 3 个测试

### D. 重复 entry 块 (2 个: comp23, comp25)

**错误**: `unable to create block named 'entry'`
**原因**: 函数生成逻辑中创建了多个 entry 块
**优先级**: 中

### E. SSA 编号错误 (1 个: comp35)

**错误**: `instruction expected to be numbered '%127'` 但得到`%126`
**原因**: 某处指令生成被跳过导致编号不连续
**优先级**: 低

### F. 全局常量未定义 (3 个: comp5, comp38, comp41)

**错误**: `use of undefined value '@M'`, `@MAX_KEYS`, `@FS_SIZE`
**需要实现**:

- 全局常量的 IR 生成
- 在模块级别生成`@name = constant i32 value`
  **优先级**: 中 - 新功能，需要实现

### G. 未定义函数 (3 个: comp42, comp43, comp45)

**错误**: `use of undefined value '@alloc'`, `@load_program'`, `@find_longest_match`
**需要实现**:

- 外部函数声明
- 函数前向声明机制
  **优先级**: 中 - 新功能

### H. Unsized 类型 (3 个: comp44, comp46, comp47)

**错误**: `Cannot allocate unsized type` - `%Agent`, `%Sphere`, `%Chromosome`
**原因**: 递归结构体定义导致类型大小无法确定
**优先级**: 低 - 语义分析问题

### I. 语法错误 (1 个: comp14)

**错误**: `expected value token` at `}`
**原因**: 块生成问题
**优先级**: 低

## 技术债务和限制

### 1. 类型系统

- bool 类型映射: 当前 i1，但某些上下文需要 i32
- 聚合类型一致性: 基本解决，但边缘情况可能存在

### 2. 短路求值

- 当前`&&`/`||`使用位运算指令
- 无法处理有副作用的右操作数
- 需要重构为 control flow 实现

### 3. 全局作用域

- 未实现全局常量生成
- 未实现外部函数声明
- 需要在模块级别添加处理

### 4. 输出验证

- 32 位链接环境缺失
- lli 解释器对大型 IR 超时
- 无法系统性验证输出正确性

## 代码质量

### 已实现的良好实践

- `generating_lvalue_`标志位机制
- 统一的`begin_block()`基本块管理
- 聚合类型指针/值区分清晰

### 需要改进

- 缺少单元测试
- 调试代码已移除但缺少日志系统
- 错误处理不够健壮（很多地方返回空字符串）

## 下一步行动计划

### 短期目标 (提升到 35+/50)

1. **实现短路求值** (comp13/15) - 预期+2
2. **修复双重指针** (comp20/33/34) - 预期+3
3. **修复重复 entry 块** (comp23/25) - 预期+2

### 中期目标 (提升到 40+/50)

4. **实现全局常量** (comp5/38/41) - 预期+3
5. **修复类型不匹配** (comp29) - 预期+1
6. **实现外部函数声明** (comp42/43/45) - 预期+3

### 长期目标

7. 修复递归类型处理
8. 完善错误处理和诊断
9. 建立输出验证流程

## 测试覆盖率

### 通过的测试类型 (31 个)

- 基础控制流 (if/while/loop)
- 数组和结构体操作
- 函数调用和返回
- 嵌套表达式
- 部分复杂算法

### 失败的测试特征

- 包含`||`/`&&`复杂条件
- 使用全局常量
- 调用外部函数
- 包含递归类型定义
- 极端边缘情况

## 性能指标

- 平均编译时间: ~1-3 秒/测试
- IR 大小: 100-2000 行
- 最大测试: comprehensive1 (1098 行 IR)

## 总结

当前实现已经完成了 IR 生成的核心功能，通过率 62%表明基础架构稳固。
剩余问题主要集中在：

1. **短路求值** - 需要重构逻辑运算符生成
2. **全局作用域** - 需要模块级别的常量和函数声明
3. **类型系统** - 需要更细致的类型转换处理

优先修复短路求值和双重指针问题，预期可将通过率提升到 70%+。
