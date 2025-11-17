#!/bin/bash

# 批量验证IR语义正确性测试套件

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/../test1/ir/verify"
VERIFY_SCRIPT="$SCRIPT_DIR/verify_ir.sh"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试计数器
TOTAL=0
PASSED=0
FAILED=0

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}  IR语义验证测试套件${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# 测试用例定义：文件名和预期返回值
declare -A TESTS
TESTS["test_simple_return.rs"]=42
TESTS["test_fibonacci.rs"]=55
TESTS["test_array_sum.rs"]=15
TESTS["test_struct_method.rs"]=25
TESTS["test_gcd.rs"]=12
TESTS["test_factorial.rs"]=120
TESTS["test_2d_array.rs"]=14
TESTS["test_struct_fields.rs"]=17
TESTS["test_methods.rs"]=50
TESTS["test_nested_if.rs"]=2
TESTS["test_impl_associated_fn.rs"]=7
TESTS["test_impl_methods.rs"]=32

# 运行每个测试
for test_file in "${!TESTS[@]}"; do
    expected="${TESTS[$test_file]}"
    full_path="$TEST_DIR/$test_file"
    
    if [ ! -f "$full_path" ]; then
        echo -e "${YELLOW}⚠ 跳过: $test_file (文件不存在)${NC}"
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    echo -e "${BLUE}[测试 $TOTAL] $test_file (预期: $expected)${NC}"
    
    # 运行验证脚本（静默输出）
    if "$VERIFY_SCRIPT" "$full_path" "$expected" > /tmp/verify_test_$$.log 2>&1; then
        echo -e "${GREEN}✅ PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}❌ FAIL${NC}"
        echo "错误详情:"
        tail -20 /tmp/verify_test_$$.log
        FAILED=$((FAILED + 1))
    fi
    echo ""
    rm -f /tmp/verify_test_$$.log
done

# 输出统计
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}           测试结果统计${NC}"
echo -e "${BLUE}=========================================${NC}"
echo -e "${GREEN}✅ 通过:${NC} $PASSED"
echo -e "${RED}❌ 失败:${NC} $FAILED"
echo -e "${BLUE}📊 总计:${NC} $TOTAL"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 所有语义验证测试通过！${NC}"
    echo -e "${GREEN}   IR不仅语法正确，而且语义正确！${NC}"
    exit 0
else
    echo -e "${RED}⚠️  有 $FAILED 个测试失败，请检查错误信息。${NC}"
    exit 1
fi
