#!/bin/bash
echo "=== 测试数组和结构体IR生成 ==="

# 测试数组
echo -n "测试 test_arrays_simple.rs... "
./build/code < test1/ir/ir_generator/test_arrays_simple.rs 2>&1 | grep -A 10000 "^define" > /tmp/test_arrays.ll
if llvm-as /tmp/test_arrays.ll -o /dev/null 2>&1; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi

# 测试结构体
echo -n "测试 test_structs_simple.rs... "
./build/code < test1/ir/ir_generator/test_structs_simple.rs 2>&1 | grep -A 10000 "^%\|^define" > /tmp/test_structs.ll
if llvm-as /tmp/test_structs.ll -o /dev/null 2>&1; then
    echo "✅ 通过"
else
    echo "❌ 失败"
fi

# 运行原有测试
echo ""
echo "=== 运行原有测试套件 ==="
./test_ir_validation.sh
