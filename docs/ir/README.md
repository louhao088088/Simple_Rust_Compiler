# IR 生成模块文档

## 概述

本模块负责将语义分析后的 AST（抽象语法树）转换为 LLVM IR（中间表示）代码。IR 生成器采用访问者模式遍历 AST，生成符合 LLVM 语法的文本格式 IR。

## 架构设计

### 核心组件

```
IRGenerator (主控制器)
    ├── IREmitter        # IR 指令发射器
    ├── TypeMapper       # 类型映射器
    ├── ValueManager     # 值管理器
    └── BuiltinTypes     # 内置类型系统
```

### 模块划分

IR 生成器按功能划分为多个独立模块：

| 文件                             | 功能                       | 文档                                      |
| -------------------------------- | -------------------------- | ----------------------------------------- |
| `ir_generator.h`                 | 主类定义和接口             | [主类设计](./01_ir_generator.md)          |
| `ir_generator_main.cpp`          | 程序入口、函数定义         | [函数与程序](./02_main_and_functions.md)  |
| `ir_generator_expressions.cpp`   | 表达式处理                 | [表达式生成](./03_expressions.md)         |
| `ir_generator_statements.cpp`    | 语句处理                   | [语句生成](./04_statements.md)            |
| `ir_generator_control_flow.cpp`  | 控制流（if/while/loop）    | [控制流](./05_control_flow.md)            |
| `ir_generator_complex_exprs.cpp` | 复杂表达式（数组、结构体） | [复杂表达式](./06_complex_expressions.md) |
| `ir_generator_builtins.cpp`      | 内置函数                   | [内置函数](./07_builtins.md)              |
| `ir_generator_helpers.cpp`       | 辅助函数                   | [辅助工具](./08_helpers.md)               |
| `ir_emitter.h/cpp`               | IR 代码发射                | [IR 发射器](./09_ir_emitter.md)           |
| `type_mapper.h/cpp`              | 类型映射                   | [类型映射](./10_type_mapper.md)           |
| `value_manager.h/cpp`            | 值管理                     | [值管理](./11_value_manager.md)           |
| `ir_generator_builtins.cpp`      | 内置函数                   | [内置函数](./07_builtins.md)              |
| `ir_generator_helpers.cpp`       | 辅助函数                   | [辅助工具](./08_helpers.md)               |
| `ir_emitter.h/cpp`               | IR 代码发射                | [IR 发射器](./09_ir_emitter.md)           |
| `type_mapper.h/cpp`              | 类型映射                   | [类型映射](./10_type_mapper.md)           |
| `value_manager.h/cpp`            | 值管理                     | [值管理](./11_value_manager.md)           |

## 设计原则

### 1. 关注点分离

- **IRGenerator**: 负责 AST 遍历和高层逻辑
- **IREmitter**: 负责底层 IR 指令生成和格式化
- **TypeMapper**: 专注于 Rust 类型到 LLVM 类型的映射
- **ValueManager**: 管理变量作用域和名称解析

### 2. 无副作用的访问者模式

- 所有 `visit()` 方法返回 `void`
- 表达式结果通过 `expr_results_` 映射存储
- 使用 `get_expr_result()` 和 `store_expr_result()` 访问结果

### 3. 目标地址传递优化

- 对于聚合类型（数组、结构体），支持"目标地址传递"
- 父表达式通过 `set_target_address()` 提供目标内存地址
- 子表达式可选择性地原地初始化，避免临时变量

### 4. 32 位平台假设

- `i32`/`u32`: 32 位整数
- `usize`/`isize`: 32 位（与指针大小一致）
- 指针: 32 位（4 字节对齐）

## 关键特性

### 1. SRET 优化（结构体返回优化）

- 对于返回结构体的函数，添加隐式 `sret` 参数
- 调用方提供返回值内存地址，被调用方直接写入
- 避免大结构体的拷贝和临时变量

### 2. Alloca 提升

- 所有 `alloca` 指令提升到函数入口块
- 使用 `%stack.N` 命名，避免与 SSA 寄存器冲突
- 减少栈帧碎片，提高内存访问效率

### 3. 聚合类型优化

- 数组/结构体通过指针传递（避免拷贝）
- 大数组初始化使用 `llvm.memset` intrinsic
- 结构体拷贝使用 `llvm.memcpy` intrinsic

### 4. 引用语义

- `&T` 和 `&mut T` 都映射为 `T*`
- 不可变引用参数添加 `noalias` 属性（优化）
- 引用类型不创建额外 alloca，直接使用指针

### 5. 副作用保证

- 所有表达式的副作用都会执行，即使结果未使用
- 连续的 `let` 语句 shadowing 不会跳过初始化表达式
- 保证函数调用的执行顺序和次数

## 编译流程

```
AST (语义分析后)
    ↓
IRGenerator::generate()
    ↓
遍历顶层 items
    ├── 常量声明 → 全局常量
    ├── 结构体定义 → LLVM 类型定义
    ├── impl 块 → 方法函数
    └── 函数定义 → LLVM 函数
    ↓
生成 main 包装器
    ↓
LLVM IR 文本输出
```

## 测试覆盖

当前通过所有 50 个综合测试用例（100% 通过率），覆盖：

- ✅ 基础类型和运算
- ✅ 数组和结构体
- ✅ 控制流（if/while/loop）
- ✅ 函数调用和递归
- ✅ 引用和可变性
- ✅ 复杂数据结构（链表、树、哈希表）
- ✅ 内存管理模拟
- ✅ 内置函数
- ✅ 类型转换
- ✅ impl 方法

## 优化技术总结

| 优化          | 效果           | 应用场景         |
| ------------- | -------------- | ---------------- |
| SRET          | 避免结构体拷贝 | 函数返回结构体   |
| Alloca 提升   | 减少栈碎片     | 所有局部变量     |
| 目标地址传递  | 避免临时变量   | 聚合类型初始化   |
| Memcpy/Memset | 批量内存操作   | 数组、结构体拷贝 |
| Noalias 属性  | LLVM 优化提示  | 可变引用参数     |
| PHI 节点合并  | 控制流融合     | if 表达式返回值  |

## 已知限制

1. **32 位平台**: 不支持 64 位指针/usize
2. **无泛型**: 不支持 Rust 泛型
3. **无 trait**: 不支持 trait 和动态分发
4. **无生命周期**: 不进行生命周期检查
5. **简化内存模型**: 不跟踪所有权和借用

## 下一步改进

1. 支持 64 位平台
2. 添加更多 LLVM 优化提示（`readonly`, `writeonly`）
3. 实现常量折叠
4. 支持内联函数
5. 添加调试信息（DWARF）

---

**最后更新**: 2025-11-23  
**测试状态**: 50/50 通过 (100%)  
**LLVM 版本**: 14.0+
