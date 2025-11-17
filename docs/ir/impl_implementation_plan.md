# impl 块和方法实现计划

## 当前状态

### ✅ 已完成

1. **Parser** - 完整支持解析：

   - `impl Type { ... }` 块
   - 方法定义（带 `&self`, `&mut self` 参数）
   - 方法调用语法：`obj.method()`
   - 关联函数调用：`Type::function()`

2. **AST 结构**：
   - `ImplBlock` 节点已定义
   - 方法调用被解析为 `CallExpr(FieldAccessExpr)`
   - 关联函数被解析为 `CallExpr(PathExpr)`

### ❌ 待实现

#### 第一阶段：语义分析

1. **ItemVisitor 支持 ImplBlock** (`src/semantic/semantic.cpp`)

   ```cpp
   void SemanticAnalyzer::visit(ImplBlock *node) {
       // 1. 解析目标类型（target_type）
       // 2. 遍历 implemented_items（方法列表）
       // 3. 为每个方法：
       //    - 如果有 self 参数，添加到符号表，类型为 target_type
       //    - 将方法注册到符号表（带类型前缀，如 Point_new, Point_distance）
       //    - 处理方法体
   }
   ```

2. **方法名称解析**
   - `Point::new` → 查找符号 `Point_new`（关联函数）
   - `obj.method()` →
     - 解析 obj 类型
     - 查找符号 `Type_method`
     - 自动插入 self 参数

#### 第二阶段：IR 生成

1. **ImplBlock 处理** (`src/ir/ir_generator.cpp`)

   ```cpp
   void IRGenerator::visit_impl_block(ImplBlock *node) {
       // 遍历方法，生成为普通函数
       // 函数名：Type_method (name mangling)
       for (auto& method : node->implemented_items) {
           // 正常函数生成，self 参数自动添加
       }
   }
   ```

2. **方法调用转换**
   - `obj.method(args)` → `Type_method(&obj, args)` IR 调用
   - `Type::func(args)` → `Type_func(args)` IR 调用

## 简化实现方案

由于 impl 块的完整实现较复杂（涉及 self 参数、方法查找、trait 等），建议**分阶段实施**：

### Phase 1: 关联函数（无 self）

```rust
impl Point {
    fn new(x: i32, y: i32) -> Point {
        Point { x: x, y: y }
    }
}

// 调用: Point::new(1, 2)
// 生成IR: call @Point_new(i32 1, i32 2)
```

**实现要点**：

- ImplBlock 中的函数按 `TypeName_FuncName` 注册
- `PathExpr` 类型的调用查找对应符号

### Phase 2: 实例方法（&self）

```rust
impl Point {
    fn distance(&self) -> i32 {
        self.x * self.x + self.y * self.y
    }
}

// 调用: p.distance()
// 生成IR: call @Point_distance(%Point* %p)
```

**实现要点**：

- 方法第一个参数自动添加 self，类型为 `&Type`（指针）
- `obj.method()` 识别为方法调用，查找 `Type_method`
- 第一个参数传递 obj 的地址

### Phase 3: 可变方法（&mut self）

```rust
impl Point {
    fn set_x(&mut self, x: i32) {
        self.x = x;
    }
}
```

## 测试用例

### test_impl_associated_fn.rs

```rust
struct Point { x: i32, y: i32 }

impl Point {
    fn new(x: i32, y: i32) -> Point {
        Point { x: x, y: y }
    }

    fn zero() -> Point {
        Point { x: 0, y: 0 }
    }
}

fn main() -> i32 {
    let p: Point = Point::new(3, 4);
    p.x + p.y  // 7
}
```

### test_impl_methods.rs

```rust
struct Point { x: i32, y: i32 }

impl Point {
    fn distance_squared(&self) -> i32 {
        self.x * self.x + self.y * self.y
    }

    fn set_x(&mut self, new_x: i32) {
        self.x = new_x;
    }
}

fn main() -> i32 {
    let mut p: Point = Point { x: 3, y: 4 };
    let d: i32 = p.distance_squared();  // 25
    p.set_x(5);
    d + p.x  // 25 + 5 = 30
}
```

## 技术挑战

1. **self 参数类型**

   - `&self` → `Type*` （指针）
   - `&mut self` → `Type*` （指针，但需要检查可变性）

2. **方法查找**

   - 需要根据接收者类型查找方法
   - 可能需要扩展符号表支持方法命名空间

3. **Name Mangling**
   - 简单方案：`TypeName_MethodName`
   - 完整方案：支持泛型、trait 等（暂不考虑）

## 实现优先级

考虑到项目时间和复杂度，建议：

1. **优先**：关联函数（Phase 1）- 相对简单，不涉及 self
2. **次要**：实例方法（Phase 2）- 核心功能，但需要处理 self
3. **可选**：可变方法（Phase 3）- 完整性，但不是必需

## 预计工作量

- **Phase 1**: 2-3 小时（语义分析 + IR 生成 + 测试）
- **Phase 2**: 3-4 小时（self 参数处理 + 方法查找 + 测试）
- **Phase 3**: 1-2 小时（在 Phase 2 基础上扩展）

**总计**: 约 6-9 小时完整实现

## 当前建议

鉴于多维数组功能已完成，建议：

1. 创建完整的测试套件和文档（多维数组）
2. 如有时间，实现 impl Phase 1（关联函数）
3. impl Phase 2/3 作为后续迭代内容
