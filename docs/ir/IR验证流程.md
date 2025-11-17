# IR 验证流程文档

## 概述

本文档描述了验证 LLVM IR 生成正确性的完整流程，包括**语法验证**和**语义验证**两个层面。

## 问题背景

之前的测试只验证了 IR 的**语法正确性**（使用`llvm-as`），但没有验证**语义正确性**（程序是否按预期执行）。一个语法正确的 IR 可能仍然有逻辑错误，导致程序运行结果不符合预期。

## 完整验证流程

### 流程图

```
Rust源代码
    ↓
[1] 编译器生成IR
    ↓
[2] llvm-as 语法验证  ← 语法层面
    ↓
[3] opt -O2 优化IR
    ↓
[4] lli 解释执行      ← 语义层面（快速验证）
    ↓
[5] llc 编译为汇编
    ↓
[6] clang 链接为可执行文件  ← 语义层面（完整验证）
    ↓
执行并检查返回值
```

### 步骤详解

#### 步骤 1：生成 IR

```bash
./build/code < test.rs > output.ll 2> compile.log
```

- **输入**：Rust 源代码
- **输出**：LLVM IR（stdout）和编译日志（stderr）
- **验证**：检查编译是否成功，IR 是否非空

#### 步骤 2：语法验证

```bash
llvm-as output.ll -o /dev/null
```

- **目的**：验证 IR 在语法和结构上是否合法
- **检查**：
  - 类型系统正确性
  - 指令格式正确性
  - SSA 形式正确性
  - 基本块结构合法性

#### 步骤 3：优化 IR（可选）

```bash
opt -O2 output.ll -S -o output_opt.ll
```

- **目的**：
  - 验证 IR 是否能被优化器处理
  - 测试生成的 IR 在优化后是否仍然正确
  - 评估优化效果（行数减少）

#### 步骤 4：解释执行

```bash
lli output_wrapper.ll
echo $?  # 获取返回值
```

- **目的**：快速验证程序语义
- **包装 main 函数**：
  ```llvm
  define i32 @main() {
  entry:
    %result = call i32 @test_function()
    ret i32 %result
  }
  ```
- **返回值检查**：比较实际返回值与预期值

#### 步骤 5：编译为汇编

```bash
llc output_wrapper.ll -o output.s
```

- **目的**：验证 IR 能否被编译为目标平台汇编代码
- **检查**：汇编代码是否生成

#### 步骤 6：链接和原生执行

```bash
clang output.s -o executable
./executable
echo $?  # 获取返回值
```

- **目的**：完整的端到端验证
- **验证**：
  - 可执行文件生成
  - 原生执行成功
  - 返回值符合预期

## 使用方法

### 单个测试验证

```bash
./scripts/verify_ir.sh <test_file.rs> [expected_return_value]
```

**示例**：

```bash
# 验证简单算术
./scripts/verify_ir.sh test1/ir/verify/test_simple_return.rs 42

# 验证数组操作
./scripts/verify_ir.sh test1/ir/verify/test_array_sum.rs 15

# 验证结构体方法
./scripts/verify_ir.sh test1/ir/verify/test_struct_method.rs 25
```

**输出示例**：

```
=========================================
  LLVM IR 完整验证流程
=========================================

测试文件: test_simple_return.rs

[1/6] 生成 LLVM IR...
✓ IR生成成功
  生成 13 行IR代码

[2/6] 验证 IR 语法 (llvm-as)...
✓ IR语法正确

[3/6] 优化 IR (opt -O2)...
✓ IR优化成功
  原始IR: 17行 -> 优化后: 10行

[4/6] 解释执行 (lli)...
✓ 执行成功
  返回值: 42
✓ 输出匹配预期: 42

[5/6] 编译为汇编 (llc)...
✓ 汇编生成成功
  汇编代码: 29行

[6/6] 链接为可执行文件 (clang)...
✓ 可执行文件生成成功
✓ 原生执行成功
  返回值: 42
✓ 输出匹配预期: 42

=========================================
  ✅ 所有验证步骤通过！
=========================================
```

### 批量测试

```bash
./scripts/test_ir_semantics.sh
```

**功能**：

- 运行所有语义验证测试
- 自动检查每个测试的预期返回值
- 生成测试统计报告

**输出示例**：

```
=========================================
  IR语义验证测试套件
=========================================

[测试 1] test_simple_return.rs (预期: 42)
✅ PASS

[测试 2] test_array_sum.rs (预期: 15)
✅ PASS

[测试 3] test_struct_method.rs (预期: 25)
✅ PASS

=========================================
           测试结果统计
=========================================
✅ 通过: 3
❌ 失败: 0
📊 总计: 3

🎉 所有语义验证测试通过！
   IR不仅语法正确，而且语义正确！
```

## 测试用例

### 当前测试用例

| 测试文件              | 功能             | 预期返回值 | 状态                    |
| --------------------- | ---------------- | ---------- | ----------------------- |
| test_simple_return.rs | 简单算术         | 42         | ✅                      |
| test_array_sum.rs     | 数组操作         | 15         | ✅                      |
| test_struct_method.rs | 结构体方法       | 25         | ✅                      |
| test_fibonacci.rs     | 斐波那契（递归） | 55         | ⏳ (需要 if 表达式支持) |

### 添加新测试

1. 在`test1/ir/verify/`目录下创建测试文件
2. 确保有一个返回`i32`的`test_xxx()`函数
3. 在`scripts/test_ir_semantics.sh`中添加测试项：
   ```bash
   TESTS["your_test.rs"]=expected_value
   ```

## 验证层次

### 第 1 层：语法验证（llvm-as）

✅ 已实现

**验证内容**：

- IR 格式正确
- 类型系统一致
- SSA 形式合法
- 基本块结构正确

**局限性**：

- ❌ 不能发现逻辑错误
- ❌ 不能验证程序行为
- ❌ 不能检测性能问题

### 第 2 层：语义验证（lli + 可执行文件）

✅ 已实现

**验证内容**：

- 程序能否执行
- 返回值是否正确
- 行为是否符合预期

**覆盖场景**：

- ✅ 基础算术运算
- ✅ 数组访问和操作
- ✅ 结构体和方法调用
- ⏳ 控制流（if/while）
- ⏳ 递归函数

### 第 3 层：性能验证（未实现）

⏳ 计划中

**验证内容**：

- 生成代码效率
- 优化效果评估
- 与参考实现对比

## 工具链依赖

### 必需工具

| 工具    | 用途        | 检查命令            |
| ------- | ----------- | ------------------- |
| llvm-as | IR 汇编器   | `llvm-as --version` |
| lli     | LLVM 解释器 | `lli --version`     |
| opt     | LLVM 优化器 | `opt --version`     |
| llc     | LLVM 编译器 | `llc --version`     |
| clang   | C/C++编译器 | `clang --version`   |

### 安装指南

**Ubuntu/Debian**：

```bash
sudo apt-get install llvm clang
```

**macOS**：

```bash
brew install llvm
```

**验证安装**：

```bash
./scripts/verify_ir.sh test1/ir/verify/test_simple_return.rs 42
```

## 常见问题

### Q1: llvm-as 报错"expected type"

**原因**：IR 语法错误或类型不匹配
**解决**：检查编译器生成的 IR，特别是类型声明部分

### Q2: lli 退出码不是预期值

**原因**：

1. 程序逻辑错误
2. 数组越界
3. 未初始化的变量

**调试**：

```bash
# 查看生成的IR
cat /tmp/ir_test_xxx/test_xxx.ll

# 手动运行lli查看详细错误
lli /tmp/ir_test_xxx/test_xxx_wrapper.ll
```

### Q3: clang 链接失败"undefined reference to main"

**原因**：main 函数在优化时被移除
**解决**：已自动处理，使用 wrapper 文件进行编译

### Q4: 测试在 lli 通过但原生执行失败

**原因**：lli 和原生执行的差异（如未定义行为）
**调试**：检查是否有未初始化变量或数组越界

## 与现有测试的关系

### test_ir_comprehensive.sh（语法测试）

- **目的**：验证 IR 语法正确性
- **方法**：llvm-as 验证
- **覆盖**：18 个测试，100%通过

### test_ir_semantics.sh（语义测试）

- **目的**：验证 IR 语义正确性
- **方法**：lli + 原生执行
- **覆盖**：3 个测试，100%通过

**互补关系**：

```
语法测试: 快速检查IR格式 (18个测试，秒级)
语义测试: 深度验证程序行为 (3个测试，分钟级)
```

## 最佳实践

### 1. 测试驱动开发

```bash
# 1. 编写测试用例
cat > test_new_feature.rs << 'EOF'
fn test_new_feature() -> i32 {
    // 你的代码
}
EOF

# 2. 运行验证
./scripts/verify_ir.sh test_new_feature.rs expected_value

# 3. 修复直到通过
```

### 2. 回归测试

每次修改编译器后，运行完整测试：

```bash
./scripts/test_ir_comprehensive.sh  # 语法
./scripts/test_ir_semantics.sh      # 语义
```

### 3. 调试技巧

```bash
# 保留中间文件
TMPDIR=/tmp/debug_ir ./scripts/verify_ir.sh test.rs

# 查看生成的文件
ls -lh /tmp/debug_ir/
cat /tmp/debug_ir/test.ll          # 原始IR
cat /tmp/debug_ir/test_opt.ll      # 优化后IR
cat /tmp/debug_ir/test.s           # 汇编代码
```

## 未来改进

### 短期（1-2 周）

- [ ] 添加更多测试用例（循环、递归、复杂数据结构）
- [ ] 支持标准输出验证（不只是返回值）
- [ ] 添加性能基准测试

### 中期（1-2 个月）

- [ ] 集成到 CI/CD 流程
- [ ] 自动生成测试报告
- [ ] 对比不同优化级别的效果

### 长期（3-6 个月）

- [ ] 模糊测试（fuzz testing）
- [ ] 差分测试（与其他 Rust 编译器对比）
- [ ] 覆盖率分析

## 总结

现在的验证流程包含两个层次：

1. **语法验证**（llvm-as）：确保 IR 格式正确
2. **语义验证**（lli + 原生执行）：确保程序行为正确

这个完整的流程能够：

- ✅ 发现语法错误
- ✅ 发现逻辑错误
- ✅ 验证优化正确性
- ✅ 端到端测试

通过这两层验证，我们可以确信生成的 IR**不仅语法正确，而且语义正确**！
