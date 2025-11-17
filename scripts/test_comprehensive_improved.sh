#!/bin/bash
# 改进的综合测试脚本 - 使用正确的diff选项
# 日期: 2025-11-15

set -e

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
COMPILER="./build/code"
TEST_DIR="TestCases/IR-1"
TIMEOUT=2
TEMP_DIR="/tmp"

# 统计变量
total=0
passed=0
failed=0
timeout_count=0

echo "=========================================="
echo "综合测试套件 - IR生成验证"
echo "=========================================="
echo ""

# 检查编译器
if [ ! -f "$COMPILER" ]; then
    echo -e "${RED}错误: 编译器不存在 $COMPILER${NC}"
    exit 1
fi

# 检查测试目录
if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}错误: 测试目录不存在 $TEST_DIR${NC}"
    exit 1
fi

# 测试每个案例
for i in {1..50}; do
    test_name="comprehensive${i}"
    test_path="${TEST_DIR}/${test_name}"
    
    if [ ! -d "$test_path" ]; then
        continue
    fi
    
    total=$((total + 1))
    
    # 生成IR并运行
    timeout ${TIMEOUT}s bash -c "
        $COMPILER < ${test_path}/${test_name}.rx 2>&1 | \
        grep -A 99999 '^; ModuleID' > ${TEMP_DIR}/${test_name}.ll && \
        lli ${TEMP_DIR}/${test_name}.ll < ${test_path}/${test_name}.in 2>&1 | \
        diff -ZB ${test_path}/${test_name}.out - > /dev/null 2>&1
    " 
    
    result=$?
    
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}✓${NC} comp${i}: PASS"
        passed=$((passed + 1))
    elif [ $result -eq 124 ]; then
        echo -e "${YELLOW}⏱${NC} comp${i}: TIMEOUT"
        timeout_count=$((timeout_count + 1))
        failed=$((failed + 1))
    else
        echo -e "${RED}✗${NC} comp${i}: FAIL"
        failed=$((failed + 1))
    fi
done

# 输出统计
echo ""
echo "=========================================="
echo "测试结果统计"
echo "=========================================="
echo -e "总计: ${total}"
echo -e "${GREEN}通过: ${passed} ($(($passed * 100 / $total))%)${NC}"
echo -e "${RED}失败: ${failed} ($(($failed * 100 / $total))%)${NC}"
echo -e "${YELLOW}超时: ${timeout_count}${NC}"
echo ""

# 返回失败数作为退出码
exit $failed
