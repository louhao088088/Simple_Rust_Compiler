#!/bin/bash

# IR-1 综合测试用例自动化脚本
# 测试 TestCases/IR-1 目录下的所有测试用例

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 计数器
TOTAL=0
PASSED=0
FAILED=0

# 临时目录
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# 清空可能存在的旧缓存文件
rm -rf /tmp/compiler_test 2>/dev/null
rm -f /tmp/comp*.ll /tmp/comp*.bc /tmp/test*.ll /tmp/test*.bc 2>/dev/null

echo "========================================="
echo "   IR-1 综合测试用例验证"
echo "========================================="
echo ""

# 编译器路径
COMPILER="./build/code"

# 检查编译器是否存在
if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}错误: 编译器不存在: $COMPILER${NC}"
    echo "请先运行: cd build && make"
    exit 1
fi

# 测试目录
TEST_DIR="./TestCases/IR-1"

# 遍历所有测试用例
for test_case in "$TEST_DIR"/comprehensive*/; do
    test_name=$(basename "$test_case")
    rx_file="$test_case/${test_name}.rx"
    in_file="$test_case/${test_name}.in"
    out_file="$test_case/${test_name}.out"
    
    # 检查文件是否存在
    if [ ! -f "$rx_file" ] || [ ! -f "$in_file" ] || [ ! -f "$out_file" ]; then
        echo -e "${YELLOW}⚠ 跳过 $test_name: 文件不完整${NC}"
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    echo -n "测试 $test_name ... "
    
    # 生成 IR（过滤掉调试输出，只保留IR代码）
    # 使用与 test_all_comprehensive.sh 完全相同的 awk 模式
    ll_file="$TEMP_DIR/${test_name}.ll"
    if ! $COMPILER < "$rx_file" 2>&1 | awk '/^%.*= type|^declare i32 @printf/,0' > "$ll_file"; then
        echo -e "${RED}❌ FAIL (IR生成失败)${NC}"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # 检查是否生成了有效的IR
    if [ ! -s "$ll_file" ]; then
        echo -e "${RED}❌ FAIL (IR为空)${NC}"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # 使用 opt 优化 IR（仅 mem2reg，提升性能）
    opt_file="$TEMP_DIR/${test_name}_opt.bc"
    if opt -mem2reg "$ll_file" -o "$opt_file" 2>/dev/null && [ -s "$opt_file" ]; then
        # opt 成功，使用优化后的 bitcode
        run_file="$opt_file"
    else
        # opt 失败，使用原始的 .ll 文件
        run_file="$ll_file"
    fi
    
    # 运行程序并比较输出（使用 lli 解释器）
    # 注意：将 stderr 重定向到 stdout，这样错误信息会导致 diff 失败
    actual_output="$TEMP_DIR/${test_name}_actual.out"
    if ! timeout 5s lli "$run_file" < "$in_file" > "$actual_output" 2>&1; then
        echo -e "${RED}❌ FAIL (运行时错误或超时)${NC}"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # 比较输出（忽略行尾空白差异）
    if diff -Z -q "$actual_output" "$out_file" > /dev/null 2>&1; then
        echo -e "${GREEN}✅ PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}❌ FAIL (输出不匹配)${NC}"
        FAILED=$((FAILED + 1))
        
        # 显示差异（可选）
        if [ "${SHOW_DIFF:-0}" = "1" ]; then
            echo "预期输出:"
            head -5 "$out_file"
            echo "实际输出:"
            head -5 "$actual_output"
            echo "---"
        fi
    fi
done

echo ""
echo "========================================="
echo "           测试结果统计"
echo "========================================="
echo -e "✅ 通过: ${GREEN}$PASSED${NC}"
echo -e "❌ 失败: ${RED}$FAILED${NC}"
echo -e "📊 总计: $TOTAL"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}⚠️  有 $FAILED 个测试失败${NC}"
    echo "提示: 设置 SHOW_DIFF=1 可以显示输出差异"
    exit 1
fi
