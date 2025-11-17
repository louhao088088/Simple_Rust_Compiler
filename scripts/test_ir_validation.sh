#!/bin/bash
# LLVM IR 验证脚本（更新版）

echo "========================================="
echo "       LLVM IR 自动化验证套件"
echo "========================================="
echo

PASS_COUNT=0
FAIL_COUNT=0

# 测试函数
run_test() {
    local test_name=$1
    local test_file=$2
    
    echo -n "测试: $test_name ... "
    
    if ./build/code < "$test_file" 2>&1 | awk '/^; ModuleID/,0' | llvm-as -o /dev/null 2>&1; then
        echo "✅ PASS"
        ((PASS_COUNT++))
    else
        echo "❌ FAIL"
        ((FAIL_COUNT++))
        echo "  错误信息:"
        ./build/code < "$test_file" 2>&1 | awk '/^; ModuleID/,0' | llvm-as -o /dev/null 2>&1 | head -5 | sed 's/^/    /'
    fi
}

# Phase 1: 基础功能测试
echo "=== Phase 1: 基础功能 ==="
run_test "基础功能 v1" "test1/ir/ir_generator/test_basic.rs"
run_test "基础功能 v2 (扩展)" "test1/ir/ir_generator/test_basic_v2.rs"
echo

# Phase 2A: if 表达式
echo "=== Phase 2A: if 表达式 ==="
run_test "if 表达式 v1" "test1/ir/ir_generator/test_if.rs"
run_test "if 表达式 v2 (扩展)" "test1/ir/ir_generator/test_if_v2.rs"
echo

# Phase 2B: while/loop 循环
echo "=== Phase 2B: 循环控制流 ==="
run_test "while/loop v1" "test1/ir/ir_generator/test_while.rs"
run_test "while/loop v2 (扩展)" "test1/ir/ir_generator/test_loops_v2.rs"
echo

# Phase 2C: 综合算法测试
echo "=== Phase 2C: 综合算法 ==="
run_test "算法实现 (斐波那契, GCD, 素数等)" "test1/ir/ir_generator/test_algorithms.rs"
echo

# 统计结果
echo "========================================="
echo "           测试结果统计"
echo "========================================="
echo "✅ 通过: $PASS_COUNT"
echo "❌ 失败: $FAIL_COUNT"
echo "📊 总计: $((PASS_COUNT + FAIL_COUNT))"
echo

if [ $FAIL_COUNT -eq 0 ]; then
    echo "🎉 所有测试通过！IR 生成器工作正常。"
    exit 0
else
    echo "⚠️  有 $FAIL_COUNT 个测试失败，请检查错误信息。"
    exit 1
fi

