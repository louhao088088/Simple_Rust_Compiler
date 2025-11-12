# IR 生成模块总览

## 一、设计哲学与核心理念

### 1.1 为什么不使用 IRBuilder?

**教育意义优先**:

- IRBuilder 是 LLVM 提供的高层封装,隐藏了 IR 构造的细节
- 手动构建 IR 能够深入理解 LLVM 的内部机制
- 更好地掌握 SSA 形式、基本块、指令的本质
- 为未来优化 pass 的编写打下基础

**手动构建的本质**:

- 直接创建 Instruction 对象并插入基本块
- 显式管理指令的操作数和类型
- 手动维护控制流图(CFG)
- 深度理解 Value-Use 关系

### 1.2 IR 生成的核心挑战

**从树到图的转换**:

- AST 是树形结构,IR 是图形结构(CFG)
- 需要将结构化控制流拆解为基本块和跳转
- 表达式求值需要遵循 SSA(Static Single Assignment)原则

**类型系统映射**:

- Rust 的复杂类型系统需要映射到 LLVM 的类型系统
- 引用语义需要转换为指针操作
- 所有权和借用在 IR 层面需要正确处理内存

**内存模型设计**:

- 栈分配(alloca)与寄存器值(SSA)的权衡
- 何时需要取地址,何时可以直接使用值
- 可变性的表达(通过内存位置实现)

## 二、整体架构设计

### 2.1 模块划分

```
src/ir/
  ├── ir_generator.h/cpp       # 主IR生成器(访问者模式)
  ├── ir_module.h/cpp          # LLVM Module的包装和管理
  ├── ir_function.h/cpp        # 函数生成器
  ├── ir_basic_block.h/cpp     # 基本块管理器
  ├── type_lowering.h/cpp      # 类型降级(Rust Type -> LLVM Type)
  ├── value_manager.h/cpp      # 值和变量的生命周期管理
  ├── instruction_builder.h/cpp # 指令手动构建辅助类
  └── ssa_builder.h/cpp        # SSA构造辅助(PHI节点等)
```

### 2.2 核心组件职责

#### IRGenerator (主控制器)

- 实现 ExprVisitor、StmtVisitor、ItemVisitor 接口
- 协调各个子模块完成 IR 生成
- 维护全局状态(当前函数、当前基本块等)
- 错误处理和诊断

#### IRModule (模块管理)

- 封装 llvm::Module
- 管理全局变量和函数声明
- 结构体类型的全局注册
- 运行时函数的声明

#### IRFunction (函数生成)

- 函数签名的创建
- 参数处理和入口块设置
- 基本块的创建和组织
- 函数验证

#### IRBasicBlock (基本块管理)

- 基本块的创建和插入
- 当前插入点的维护
- Terminator 指令的检查
- 基本块的合法性验证

#### TypeLowering (类型转换)

- Rust 类型到 LLVM 类型的映射
- 复合类型的布局计算
- 类型缓存避免重复创建

#### ValueManager (值管理)

- 变量到 LLVM Value 的映射
- 作用域管理
- SSA 值的生命周期跟踪
- alloca 指令的集中管理

#### InstructionBuilder (指令构建)

- 不依赖 IRBuilder 的指令创建
- 各类指令的统一构建接口
- 操作数类型检查
- 指令插入点管理

#### SSABuilder (SSA 构造)

- PHI 节点的创建和管理
- 控制流合并点的值合并
- 不完整 PHI 的延迟填充
- SSA 形式的正确性保证

## 三、手动构建 IR 的核心技术

### 3.1 指令创建的基本模式

**不使用 IRBuilder 的指令创建**:

```
传统方式(IRBuilder):
  Value* result = builder.CreateAdd(lhs, rhs, "add");

手动方式:
  Instruction* add_inst = BinaryOperator::Create(
    Instruction::Add, lhs, rhs, "add"
  );
  current_block->getInstList().push_back(add_inst);
  return add_inst;
```

**关键点**:

- 直接使用 Instruction 的静态 Create 方法
- 手动插入到 BasicBlock 的指令链表中
- 显式管理指令的命名和类型

### 3.2 基本块的手动管理

**基本块创建流程**:

1. 使用`BasicBlock::Create(context, name, function)`创建
2. 维护"当前插入块"指针
3. 指令直接 append 到当前块的指令列表
4. 确保每个基本块有且仅有一个 terminator

**插入点管理**:

- 维护`current_insert_block_`指针
- 控制流改变时切换插入点
- 检查当前块是否已 sealed(有 terminator)

### 3.3 控制流的手动构造

**if 表达式的基本块组织**:

```
entry:
  %cond = ... (条件计算)
  br %cond, label %then, label %else

then:
  %then_val = ... (then分支计算)
  br label %merge

else:
  %else_val = ... (else分支计算)
  br label %merge

merge:
  %result = phi [%then_val, %then], [%else_val, %else]
  ... (后续代码)
```

**关键技术**:

- 提前创建所有需要的基本块
- 按顺序生成每个块的内容
- 使用 PHI 节点合并控制流
- 处理块的前驱-后继关系

### 3.4 SSA 形式的手动维护

**SSA 的本质**:

- 每个变量只能赋值一次
- 控制流合并需要 PHI 节点
- 所有使用必须被单一定义支配

**实现策略**:

- 变量用 alloca+load/store 实现(非 SSA)
- 临时值直接用 SSA 形式
- 表达式结果是 SSA 值
- PHI 节点在控制流合并点手动创建

## 四、类型系统映射

### 4.1 基本类型映射规则

| Rust 类型 | LLVM 类型 | 说明                             |
| --------- | --------- | -------------------------------- |
| i32       | i32       | 32 位有符号整数                  |
| u32       | i32       | 32 位无符号整数(LLVM 不区分符号) |
| i64/u64   | i64       | 64 位整数                        |
| bool      | i1        | 1 位布尔值                       |
| char      | i32       | Unicode 标量值                   |
| ()        | void      | 单元类型                         |
| !         | void      | Never 类型(函数不返回)           |

### 4.2 复合类型映射

**数组类型**:

- Rust: `[T; N]`
- LLVM: `[N x T_llvm]`
- 需要递归转换元素类型

**结构体类型**:

- Rust: `struct Point { x: i32, y: i32 }`
- LLVM: `%Point = type { i32, i32 }`
- 需要在 Module 级别声明结构体类型
- 字段顺序必须严格对应

**引用类型**:

- Rust: `&T` 或 `&mut T`
- LLVM: `T*` (指针)
- 可变性在 IR 层面不做区分(在语义分析已检查)

**函数类型**:

- Rust: `fn(i32, i32) -> i32`
- LLVM: `i32 (i32, i32)*` (函数指针类型)
- 需要转换参数类型和返回类型

### 4.3 类型转换的实现策略

**类型缓存机制**:

- 避免为同一 Rust 类型重复创建 LLVM 类型
- 使用`std::unordered_map<Type*, llvm::Type*>`缓存
- 递归类型需要特殊处理(先创建 opaque type,再填充)

**结构体类型声明**:

- 第一遍:收集所有结构体名称,创建 opaque struct
- 第二遍:填充 struct body(字段类型)
- 避免循环依赖问题

## 五、内存模型设计

### 5.1 alloca vs SSA 值

**使用 alloca 的场景**:

- let 绑定的变量(可能需要取地址)
- 可变变量(需要多次赋值)
- 函数参数(为了统一处理,参数值 store 到 alloca)
- 需要在闭包中捕获的变量

**使用 SSA 值的场景**:

- 表达式的中间结果
- 临时计算值
- 不可变的纯计算

**设计原则**:

- 所有 let 绑定都用 alloca(简化实现)
- 表达式求值返回 SSA 值
- 变量访问通过 load 从 alloca 读取
- 赋值通过 store 写入 alloca

### 5.2 内存操作的手动构建

**alloca 指令**:

```cpp
// 在函数entry块开始处创建
AllocaInst* alloca = new AllocaInst(
    type,              // 分配的类型
    0,                 // 地址空间(通常是0)
    nullptr,           // 数组大小(nullptr表示单个元素)
    alignment,         // 对齐要求
    name,              // 指令名称
    entry_block        // 插入位置
);
```

**load 指令**:

```cpp
LoadInst* load = new LoadInst(
    value_type,        // 加载的类型
    ptr_value,         // 要加载的指针
    name,              // 指令名称
    is_volatile,       // 是否volatile
    current_block      // 插入位置
);
```

**store 指令**:

```cpp
StoreInst* store = new StoreInst(
    value,             // 要存储的值
    ptr,               // 存储的目标地址
    is_volatile,       // 是否volatile
    current_block      // 插入位置
);
```

### 5.3 变量生命周期管理

**作用域进入**:

- 创建新的变量映射表层
- 继承外层作用域的可见性

**变量定义**:

- 创建 alloca 指令
- 记录到当前作用域的映射表
- 如果有初始值,生成 store 指令

**变量查找**:

- 从当前作用域向外层递归查找
- 返回对应的 alloca 指令
- 访问时生成 load 指令

**作用域退出**:

- 弹出当前作用域的映射表
- alloca 的内存由函数返回时自动释放

## 六、表达式生成策略

### 6.1 字面量表达式

**整数字面量**:

```cpp
llvm::Value* visit(LiteralExpr* node) {
    if (node->kind == LiteralKind::INTEGER) {
        llvm::Type* int_type = type_lowering_.convert(node->type);
        return llvm::ConstantInt::get(int_type, node->int_value);
    }
    // ...
}
```

**布尔字面量**:

- 使用`ConstantInt::get(Type::getInt1Ty(context), value)`
- true -> 1, false -> 0

**字符字面量**:

- 按 i32 处理(Unicode 标量值)

**字符串字面量**:

- 创建全局常量数组
- 返回指向数组首元素的指针

### 6.2 变量表达式

**变量访问**:

1. 从 ValueManager 查找变量对应的 alloca
2. 手动创建 LoadInst 从 alloca 读取值
3. 返回 load 指令的结果

**左值 vs 右值**:

- 右值上下文:生成 load 指令,返回值
- 左值上下文(赋值目标):直接返回 alloca 指针
- 通过上下文标志位区分

### 6.3 二元运算表达式

**算术运算**:

- 递归生成左右操作数
- 根据操作符类型创建对应的 BinaryOperator
- 手动插入指令到当前基本块

```cpp
llvm::Value* visit(BinaryExpr* node) {
    llvm::Value* lhs = node->left->accept(this);
    llvm::Value* rhs = node->right->accept(this);

    Instruction* inst = nullptr;
    switch (node->op) {
        case TokenType::PLUS:
            inst = BinaryOperator::Create(
                Instruction::Add, lhs, rhs, "add"
            );
            break;
        case TokenType::MINUS:
            inst = BinaryOperator::Create(
                Instruction::Sub, lhs, rhs, "sub"
            );
            break;
        // ...
    }

    current_block_->getInstList().push_back(inst);
    return inst;
}
```

**比较运算**:

- 使用 ICmpInst(整数比较)
- 手动指定比较谓词(EQ, NE, SLT, SGT 等)
- 有符号数用 S 前缀,无符号数用 U 前缀

### 6.4 一元运算表达式

**取负运算**:

- 创建`Sub 0, operand`指令
- 或使用`Neg`指令

**逻辑非**:

- 创建`Xor operand, true`指令
- 或使用`Not`指令

**解引用**:

- 如果操作数是引用类型,生成 load 指令
- 返回解引用后的值

### 6.5 赋值表达式

**赋值的本质**:

- 左边必须是左值(变量、数组索引、字段访问)
- 右边是右值(任意表达式)
- 生成 store 指令将右值写入左值地址

**实现步骤**:

1. 以左值模式生成左边表达式(得到地址)
2. 以右值模式生成右边表达式(得到值)
3. 创建 StoreInst 将值写入地址
4. 返回被赋的值(赋值表达式自身也有值)

## 七、语句生成策略

### 7.1 块语句

**基本流程**:

1. 不创建新基本块(块语句本身不改变控制流)
2. 进入新作用域
3. 顺序生成每条语句
4. 退出作用域
5. 如果块有返回值,返回最后一个表达式的值

**特殊处理**:

- 检查块内是否有提前返回
- 处理 unreachable 代码

### 7.2 let 语句

**let 绑定的生成**:

1. 为变量创建 alloca 指令(在 entry 块)
2. 如果有初始化表达式,生成其 IR
3. 创建 store 指令将初始值写入 alloca
4. 记录变量到 ValueManager

**模式匹配**:

- 简单标识符:直接绑定
- 元组/结构体模式:递归解构
- 需要生成多个 alloca 和 store

### 7.3 return 语句

**返回值处理**:

1. 生成返回表达式的 IR(如果有)
2. 手动创建 ReturnInst
3. 标记当前基本块已 sealed(有 terminator)
4. 后续代码在新基本块中生成(如果有)

**无返回值函数**:

- 创建`ReturnInst::Create(context, nullptr, block)`
- 对应 Rust 的`-> ()`

### 7.4 表达式语句

**纯副作用**:

- 生成表达式的 IR
- 丢弃返回值(不使用)
- 确保副作用(函数调用等)被执行

## 八、控制流生成

### 8.1 if 表达式

**基本块组织**:

```
current_block:
  ... (前面的代码)
  %cond = ... (生成条件表达式)
  [创建then_block, else_block, merge_block]
  创建BranchInst到then或else

then_block:
  ... (生成then分支)
  如果没有提前返回,创建br到merge

else_block:
  ... (生成else分支)
  如果没有提前返回,创建br到merge

merge_block:
  如果两个分支都有值,创建PHI节点
  ... (后续代码)
```

**手动创建分支指令**:

```cpp
// 条件分支
BranchInst* cond_br = BranchInst::Create(
    then_block,   // true目标
    else_block,   // false目标
    cond_value,   // 条件
    current_block // 插入位置
);

// 无条件分支
BranchInst* br = BranchInst::Create(
    target_block,  // 目标
    current_block  // 插入位置
);
```

**PHI 节点创建**:

```cpp
PHINode* phi = PHINode::Create(
    result_type,   // 结果类型
    2,             // 预期incoming数量
    "if.result",   // 名称
    merge_block    // 插入位置
);
phi->addIncoming(then_value, then_block);
phi->addIncoming(else_value, else_block);
```

### 8.2 loop 表达式

**无限循环结构**:

```
current_block:
  ... (前面的代码)
  br label %loop_header

loop_header:
  ... (循环体)
  如果没有break,br回loop_header

loop_exit:
  ... (循环后的代码)
```

**break/continue 支持**:

- 维护循环目标栈
- break 生成跳转到 loop_exit
- continue 生成跳转到 loop_header

**带标签的循环**:

- 支持 break 'label
- 维护标签到目标块的映射

### 8.3 while 表达式

**条件循环结构**:

```
current_block:
  br label %loop_header

loop_header:
  %cond = ... (生成条件)
  br %cond, label %loop_body, label %loop_exit

loop_body:
  ... (循环体)
  br label %loop_header

loop_exit:
  ... (循环后的代码)
```

**优化要点**:

- 条件在 header 中评估
- body 结束后跳回 header 重新检查
- 支持 first iteration optimization

### 8.4 match 表达式

**匹配表达式结构**:

```
current_block:
  %scrutinee = ... (生成被匹配表达式)

arm1_check:
  ... (生成模式1的检查)
  br %match, label %arm1_body, label %arm2_check

arm1_body:
  ... (分支1的代码)
  br label %merge

arm2_check:
  ... (生成模式2的检查)
  br %match, label %arm2_body, label %arm3_check

...

merge:
  %result = phi [%arm1_val, %arm1_body], [%arm2_val, %arm2_body], ...
```

**模式匹配的实现**:

- 字面量模式:简单比较
- 变量模式:直接绑定
- 结构体模式:逐字段匹配
- 枚举模式:tag 检查+数据提取

## 九、函数生成

### 9.1 函数声明

**创建函数对象**:

```cpp
// 1. 转换函数类型
llvm::FunctionType* fn_type = /* 从FnDecl转换 */;

// 2. 创建Function对象
llvm::Function* func = llvm::Function::Create(
    fn_type,
    llvm::Function::ExternalLinkage,  // 链接类型
    node->name,                        // 函数名
    module_->get_llvm_module()        // 所属module
);

// 3. 设置参数名称
size_t idx = 0;
for (auto& arg : func->args()) {
    arg.setName(node->params[idx++].name);
}
```

**函数签名转换**:

- 参数类型列表:递归转换每个参数类型
- 返回类型:转换返回类型(void 表示 unit)
- 可变参数:LLVM 支持但 Rust 不常用

### 9.2 函数体生成

**entry 基本块设置**:

```cpp
// 1. 创建entry块
llvm::BasicBlock* entry = llvm::BasicBlock::Create(
    context, "entry", func
);
current_block_ = entry;

// 2. 为每个参数创建alloca并store
for (auto& arg : func->args()) {
    AllocaInst* alloca = create_alloca_in_entry(
        arg.getType(), arg.getName()
    );
    StoreInst* store = new StoreInst(&arg, alloca, entry);
    value_manager_->define_variable(
        arg.getName(), alloca, /* is_mutable */
    );
}

// 3. 生成函数体
node->body->accept(this);

// 4. 确保有返回指令
if (!current_block_->getTerminator()) {
    if (/* return type is void */) {
        ReturnInst::Create(context, nullptr, current_block_);
    } else {
        // 错误:非void函数缺少return
        new UnreachableInst(context, current_block_);
    }
}
```

**alloca 提升技术**:

- 所有 alloca 应该在 entry 块的开始
- 后续生成的 alloca 需要插入到 entry 开头
- 维护 entry_insert_point 指针

### 9.3 函数调用

**call 指令手动构建**:

```cpp
// 1. 查找被调用函数
llvm::Function* callee = /* 从symbol table查找 */;

// 2. 生成实参
std::vector<llvm::Value*> args;
for (auto* arg_expr : node->arguments) {
    args.push_back(arg_expr->accept(this));
}

// 3. 创建CallInst
llvm::CallInst* call = llvm::CallInst::Create(
    callee->getFunctionType(),  // 函数类型
    callee,                      // 被调用函数
    args,                        // 参数列表
    "call",                      // 名称
    current_block_               // 插入位置
);

return call;  // call指令本身就是返回值
```

**间接调用**(函数指针):

- 第一个参数是 Value*(函数指针)而不是 Function*
- 需要显式提供 FunctionType

## 十、复合类型处理

### 10.1 数组

**数组类型表示**:

- LLVM: `[N x element_type]`
- 在栈上分配:`alloca [N x T]`

**数组字面量**:

```cpp
// let arr = [1, 2, 3];
// 1. 创建数组alloca
AllocaInst* arr_alloca = new AllocaInst(
    llvm::ArrayType::get(i32_type, 3),
    0, "arr", entry_block
);

// 2. 逐元素初始化
for (size_t i = 0; i < 3; ++i) {
    // 计算元素地址: GEP arr, 0, i
    llvm::Value* indices[] = {
        llvm::ConstantInt::get(i64_type, 0),
        llvm::ConstantInt::get(i64_type, i)
    };
    GetElementPtrInst* gep = GetElementPtrInst::Create(
        array_type, arr_alloca, indices, "elem.ptr"
    );
    current_block_->getInstList().push_back(gep);

    // 存储元素值
    llvm::Value* elem_val = /* 元素表达式 */;
    StoreInst* store = new StoreInst(elem_val, gep, current_block_);
}
```

**数组索引**:

```cpp
// arr[i]
// 1. 生成数组指针
llvm::Value* arr_ptr = /* 变量查找得到alloca */;

// 2. 生成索引值
llvm::Value* idx = node->index->accept(this);

// 3. 创建GEP指令
llvm::Value* indices[] = {
    llvm::ConstantInt::get(i64_type, 0),  // 先解引用指针
    idx                                    // 再索引数组
};
GetElementPtrInst* gep = GetElementPtrInst::Create(
    array_type, arr_ptr, indices, "arr.elem"
);
current_block_->getInstList().push_back(gep);

// 4. 如果是右值,load元素
if (/* 右值上下文 */) {
    LoadInst* load = new LoadInst(
        elem_type, gep, "arr.elem.val", current_block_
    );
    return load;
} else {
    return gep;  // 左值返回地址
}
```

### 10.2 结构体

**结构体类型声明**:

```cpp
// struct Point { x: i32, y: i32 }
// 1. 在Module级别声明StructType
llvm::StructType* point_type = llvm::StructType::create(
    context, "Point"
);

// 2. 设置body(字段类型)
std::vector<llvm::Type*> fields = {
    i32_type,  // x
    i32_type   // y
};
point_type->setBody(fields);

// 3. 缓存类型
type_cache_[rust_struct_type] = point_type;
```

**结构体初始化**:

```cpp
// Point { x: 10, y: 20 }
// 1. 创建结构体alloca
AllocaInst* struct_alloca = new AllocaInst(
    point_type, 0, "point", entry_block
);

// 2. 初始化每个字段
for (size_t i = 0; i < fields.size(); ++i) {
    // 计算字段地址: GEP struct_ptr, 0, i
    llvm::Value* indices[] = {
        llvm::ConstantInt::get(i32_type, 0),
        llvm::ConstantInt::get(i32_type, i)
    };
    GetElementPtrInst* gep = GetElementPtrInst::Create(
        point_type, struct_alloca, indices, "field.ptr"
    );
    current_block_->getInstList().push_back(gep);

    // 存储字段值
    llvm::Value* field_val = /* 字段表达式 */;
    new StoreInst(field_val, gep, current_block_);
}
```

**字段访问**:

```cpp
// point.x
// 1. 获取结构体指针
llvm::Value* struct_ptr = /* 变量查找 */;

// 2. 查找字段索引
size_t field_idx = /* 从StructType查找字段名对应的索引 */;

// 3. 创建GEP
llvm::Value* indices[] = {
    llvm::ConstantInt::get(i32_type, 0),
    llvm::ConstantInt::get(i32_type, field_idx)
};
GetElementPtrInst* gep = GetElementPtrInst::Create(
    struct_type, struct_ptr, indices, "field.ptr"
);
current_block_->getInstList().push_back(gep);

// 4. 右值load,左值返回地址
if (/* 右值 */) {
    return new LoadInst(field_type, gep, "field.val", current_block_);
} else {
    return gep;
}
```

### 10.3 引用和指针

**引用的本质**:

- 在 IR 层面就是指针
- `&T`和`&mut T`都是`T*`
- 可变性在类型检查阶段已验证

**取引用操作**:

```cpp
// &x 或 &mut x
// 1. 以左值模式生成被引用表达式(得到地址)
llvm::Value* addr = generate_lvalue(node->expr);

// 2. 直接返回地址(不需要额外指令)
return addr;
```

**解引用操作**:

```cpp
// *ptr
// 1. 生成指针值
llvm::Value* ptr = node->expr->accept(this);

// 2. Load从指针读取值
LoadInst* load = new LoadInst(
    pointee_type, ptr, "deref", current_block_
);
return load;
```

**原始指针**:

- `*const T`和`*mut T`与引用类似
- 可以进行指针运算(需要额外支持)

## 十一、特殊处理

### 11.1 main 函数

**main 函数签名调整**:

- Rust 的 main: `fn main()`
- C 的 main: `int main(int argc, char** argv)`
- 需要创建 wrapper 或直接映射

**实现选择**:

1. 直接生成`i32 @main()`,内部调用`void @rust_main()`
2. 让 codegen 的 main 直接返回 i32(0 表示成功)

### 11.2 常量

**const item 的处理**:

- 编译期已求值(ConstEvaluator)
- 在 IR 中用 LLVM Constant 表示
- 直接内联使用,不生成变量

**全局常量**:

```cpp
llvm::Constant* init_value = /* 常量值 */;
llvm::GlobalVariable* gv = new llvm::GlobalVariable(
    *module_,
    value_type,
    true,  // is constant
    llvm::GlobalValue::InternalLinkage,
    init_value,
    const_name
);
```

### 11.3 标准库函数声明

**需要声明的运行时函数**:

- `printf`/`puts`:输出
- `malloc`/`free`:动态内存(如果支持 Box)
- `panic`:panic 处理

**声明方式**:

```cpp
// 声明printf
llvm::FunctionType* printf_type = llvm::FunctionType::get(
    i32_type,
    {ptr_type},  // char* format
    true  // 可变参数
);
llvm::Function* printf_fn = llvm::Function::Create(
    printf_type,
    llvm::Function::ExternalLinkage,
    "printf",
    module_
);
```

### 11.4 类型转换(as)

**整数类型转换**:

- 窄化:`trunc i64 to i32`
- 扩展:
  - 有符号:`sext i32 to i64`
  - 无符号:`zext i32 to i64`

**实现**:

```cpp
llvm::Value* visit(AsExpr* node) {
    llvm::Value* val = node->expr->accept(this);
    llvm::Type* from_type = val->getType();
    llvm::Type* to_type = type_lowering_.convert(node->target_type);

    if (from_type == to_type) {
        return val;  // 无需转换
    }

    // 整数转换
    if (from_type->isIntegerTy() && to_type->isIntegerTy()) {
        unsigned from_bits = from_type->getIntegerBitWidth();
        unsigned to_bits = to_type->getIntegerBitWidth();

        if (from_bits > to_bits) {
            // 截断
            TruncInst* trunc = new TruncInst(
                val, to_type, "trunc", current_block_
            );
            return trunc;
        } else if (from_bits < to_bits) {
            // 扩展(需要判断有无符号)
            if (/* 有符号 */) {
                SExtInst* sext = new SExtInst(
                    val, to_type, "sext", current_block_
                );
                return sext;
            } else {
                ZExtInst* zext = new ZExtInst(
                    val, to_type, "zext", current_block_
                );
                return zext;
            }
        }
    }

    // 其他转换类型...
}
```

## 十二、实现步骤规划

### 第一阶段:基础框架(1-2 周)

**目标**:搭建基本架构,能生成最简单的 IR

**任务清单**:

1. 创建 ir 模块目录结构
2. 实现 IRModule 类(封装 llvm::Module)
3. 实现 TypeLowering 类(支持基本类型)
4. 实现 ValueManager 类(作用域管理)
5. 实现 IRGenerator 骨架(空的 visitor 方法)
6. 配置 CMakeLists.txt 链接 LLVM 库
7. 测试:能生成空 Module 并输出

### 第二阶段:简单表达式和语句(2-3 周)

**目标**:支持算术表达式、变量、赋值

**任务清单**:

1. 实现字面量表达式生成(整数、布尔)
2. 实现二元运算(+, -, \*, /, %)
3. 实现比较运算(==, !=, <, >, <=, >=)
4. 实现 let 语句(alloca + store)
5. 实现变量表达式(load)
6. 实现赋值表达式(store)
7. 实现块表达式/语句
8. 测试:简单的算术计算程序

### 第三阶段:控制流(2-3 周)

**目标**:支持 if、loop、while

**任务清单**:

1. 实现基本块创建和管理
2. 实现条件分支指令(BranchInst)
3. 实现 if 表达式(含 PHI 节点)
4. 实现 loop 表达式
5. 实现 while 表达式
6. 实现 break/continue 语句
7. 实现 return 语句
8. 测试:斐波那契、阶乘等算法

### 第四阶段:函数(2-3 周)

**目标**:支持函数定义和调用

**任务清单**:

1. 实现函数声明生成
2. 实现函数体生成(entry 块设置)
3. 实现参数处理
4. 实现函数调用(CallInst)
5. 实现返回值处理
6. 实现 main 函数特殊处理
7. 测试:多函数程序

### 第五阶段:复合类型(3-4 周)

**目标**:支持数组和结构体

**任务清单**:

1. 扩展 TypeLowering 支持数组类型
2. 实现数组字面量生成
3. 实现数组索引(GEP 指令)
4. 扩展 TypeLowering 支持结构体
5. 实现结构体声明注册
6. 实现结构体初始化
7. 实现字段访问(GEP 指令)
8. 测试:复杂数据结构

### 第六阶段:引用和类型转换(1-2 周)

**目标**:完善类型系统

**任务清单**:

1. 实现引用类型转换
2. 实现取引用操作
3. 实现解引用操作
4. 实现 as 类型转换(trunc/sext/zext)
5. 测试:引用语义

### 第七阶段:优化和完善(持续)

**目标**:提升生成质量

**任务清单**:

1. alloca 提升到 entry 块开始
2. 死代码消除(unreachable 后的代码)
3. 简单的常量折叠
4. 运行 LLVM 优化 pass
5. 完善错误处理
6. 添加调试信息生成(DIBuilder)

## 十三、测试策略

### 13.1 单元测试

**测试内容**:

- TypeLowering:每种 Rust 类型的转换正确性
- 表达式生成:每种表达式生成的 IR 正确性
- 语句生成:每种语句生成的 IR 正确性
- 控制流:基本块组织的正确性

**测试方法**:

- 创建最小 AST 节点
- 调用相应的 visit 方法
- 检查生成的 LLVM IR
- 使用 llvm::verifyModule 验证合法性

### 13.2 集成测试

**测试内容**:

- 完整的小程序
- 组合使用多种特性
- 边界情况和错误处理

**测试方法**:

- 使用 testcases 目录的 Rust 程序
- 运行完整编译流程
- 生成.ll 文件
- 使用 lli 执行并检查输出
- 对比预期结果

### 13.3 验证工具

**LLVM 提供的验证**:

```cpp
#include <llvm/IR/Verifier.h>

// 验证Module
if (llvm::verifyModule(*module_, &llvm::errs())) {
    llvm::errs() << "Error: Module verification failed\n";
    return false;
}

// 验证Function
if (llvm::verifyFunction(*func, &llvm::errs())) {
    llvm::errs() << "Error: Function verification failed\n";
    return false;
}
```

**外部工具**:

- `llvm-as output.ll`:检查语法
- `llvm-dis output.bc`:反汇编 bitcode
- `opt -verify output.ll`:运行验证 pass
- `lli output.ll`:直接执行
- `llc output.ll -o output.s`:生成汇编

### 13.4 调试技巧

**打印 IR**:

```cpp
module_->print(llvm::outs(), nullptr);  // 打印到stdout
module_->print(llvm::errs(), nullptr);  // 打印到stderr

// 打印到文件
std::error_code ec;
llvm::raw_fd_ostream out("output.ll", ec);
module_->print(out, nullptr);
```

**可视化 CFG**:

```cpp
// 为每个函数生成CFG dot图
for (auto& func : module_->functions()) {
    func.viewCFG();  // 需要graphviz
}
```

**LLVM 验证器**:

- 自动检查 SSA 形式
- 检查类型一致性
- 检查基本块 terminator
- 检查 PHI 节点合法性

## 十四、常见问题与解决方案

### 14.1 基本块没有 terminator

**问题表现**:

- 验证失败:"Basic block doesn't have a terminator"

**原因**:

- 生成代码时忘记添加 br/ret/unreachable
- 控制流分支遗漏

**解决方案**:

- 每次创建基本块后,确保最终添加 terminator
- 在切换插入点前检查当前块是否已 sealed
- 使用辅助函数自动检查

### 14.2 PHI 节点前驱不匹配

**问题表现**:

- 验证失败:"PHI node operands are not the same type"
- 或"PHI node has multiple entries for the same basic block"

**原因**:

- PHI 的 incoming 块与实际前驱不一致
- 控制流改变后 PHI 未更新

**解决方案**:

- 在添加 PHI incoming 时,使用当前实际的基本块
- 生成控制流时,记录每个分支的结束块
- 延迟 PHI 的填充,直到所有分支完成

### 14.3 GEP 指令索引错误

**问题表现**:

- 运行时访问错误
- 或验证失败:"Invalid GetElementPtr indices"

**原因**:

- GEP 第一个索引应该是 0(解引用指针)
- 数组/结构体索引从第二个参数开始

**解决方案**:

```cpp
// 正确的数组GEP
llvm::Value* indices[] = {
    ConstantInt::get(i64_type, 0),  // 解引用alloca指针
    index_value                      // 数组索引
};

// 正确的结构体GEP
llvm::Value* indices[] = {
    ConstantInt::get(i32_type, 0),   // 解引用指针
    ConstantInt::get(i32_type, field_idx)  // 字段索引
};
```

### 14.4 类型不匹配

**问题表现**:

- 验证失败:"Instruction type does not match operand type"

**原因**:

- load/store 的类型与指针的 pointee 类型不一致
- 函数调用的参数类型不匹配

**解决方案**:

- 严格使用 TypeLowering 转换类型
- 在创建指令前检查操作数类型
- 必要时插入 cast 指令

### 14.5 作用域管理错误

**问题表现**:

- 变量查找失败
- 或访问了已销毁的变量

**原因**:

- enter_scope/exit_scope 不配对
- 提前退出时未清理作用域

**解决方案**:

- 使用 RAII 封装作用域管理
- 在异常路径也确保清理
- 单元测试作用域逻辑

## 十五、性能优化方向

### 15.1 减少 alloca 使用

**优化策略**:

- 不可变绑定直接用 SSA 值
- 只有可变变量才 alloca
- 简单的临时变量用寄存器

**实施方法**:

- 在语义分析时标记"可能需要地址"的变量
- IR 生成时根据标记决定 alloca 或 SSA

### 15.2 LLVM 优化 Pass

**使用 LLVM 的优化**:

```cpp
#include <llvm/Passes/PassBuilder.h>

llvm::PassBuilder pb;
llvm::LoopAnalysisManager lam;
llvm::FunctionAnalysisManager fam;
llvm::CGSCCAnalysisManager cgam;
llvm::ModuleAnalysisManager mam;

pb.registerModuleAnalyses(mam);
pb.registerFunctionAnalyses(fam);
pb.registerLoopAnalyses(lam);
pb.registerCGSCCAnalyses(cgam);
pb.crossRegisterProxies(lam, fam, cgam, mam);

// 运行O2优化
llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(
    llvm::OptimizationLevel::O2
);
mpm.run(*module_, mam);
```

**常用优化**:

- mem2reg:alloca 提升为 SSA
- simplifycfg:简化控制流
- dce:死代码消除
- constprop:常量传播

### 15.3 指令选择优化

**模式识别**:

- 识别常见模式并生成更优指令
- 例如:`x = x + 1` -> 使用 add 指令而不是 load-add-store

### 15.4 控制流优化

**优化技巧**:

- 合并只有一条语句的基本块
- 消除空的基本块
- 条件常量时直接消除分支

## 十六、扩展方向

### 16.1 调试信息生成

**使用 DIBuilder**:

- 生成源码位置信息
- 变量名保留
- 类型信息保留
- 支持 GDB/LLDB 调试

### 16.2 更多 Rust 特性

**未来支持**:

- 枚举和模式匹配完整实现
- trait 和动态分发
- 泛型的单态化
- 闭包(捕获环境)
- 迭代器

### 16.3 代码生成优化

**后端优化**:

- 寄存器分配启发式
- 指令调度
- 循环展开
- 向量化

### 16.4 目标平台支持

**交叉编译**:

- x86-64
- ARM
- RISC-V
- WebAssembly

## 十七、学习资源

### 17.1 LLVM 官方文档

- LLVM Language Reference Manual
- LLVM Programmer's Manual
- Writing an LLVM Pass

### 17.2 教程和书籍

- "LLVM Cookbook"
- "Getting Started with LLVM Core Libraries"
- Kaleidoscope Tutorial (LLVM 官方教程)

### 17.3 开源项目参考

- Rust 编译器(rustc)的 LLVM IR 生成部分
- Clang 的 CodeGen 模块
- 其他 LLVM frontend 实现

## 十八、总结

手动构建 LLVM IR 是一个深入理解编译器后端和 IR 设计的绝佳途径。通过不使用 IRBuilder,你将:

1. **深入理解 LLVM IR 的结构**

   - 每条指令的具体语义
   - 基本块和控制流的组织
   - SSA 形式的维护

2. **掌握关键编译技术**

   - 类型 lowering
   - 控制流转换
   - 内存模型设计

3. **建立扎实的基础**
   - 为后续优化 pass 编写准备
   - 理解编译器后端工作原理
   - 可以迁移到其他 IR 系统

**实施建议**:

- 从最简单的特性开始
- 每个阶段充分测试
- 参考 LLVM 文档和示例
- 使用验证工具确保正确性
- 不断重构提升代码质量

祝你的 IR 生成模块实现顺利!
