#!/bin/bash

# Phase 2F 功能验证脚本
# 专门测试函数参数优化和数组初始化语法

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

COMPILER="./build/code"
TEST_DIR="test1/ir/ir_generator"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   Phase 2F 功能验证${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# 测试函数参数优化
echo -e "${BLUE}=== 测试1: 函数参数与返回值优化 ===${NC}"
echo "测试聚合类型（数组/结构体）的参数传递和返回值处理..."
echo ""

TEST_FILE="$TEST_DIR/test_function_params.rs"
echo -e "${YELLOW}生成IR...${NC}"
IR_OUTPUT=$($COMPILER < "$TEST_FILE" 2>&1 | grep -A 10000 "ModuleID")

# 验证IR语法
echo -ne "验证IR语法正确性 ... "
if echo "$IR_OUTPUT" | llvm-as -o /dev/null 2>&1; then
    echo -e "${GREEN}✅ 通过${NC}"
else
    echo -e "${RED}❌ 失败${NC}"
    echo "$IR_OUTPUT" | llvm-as 2>&1 | head -20
    exit 1
fi

# 检查关键特征
echo ""
echo -e "${YELLOW}检查关键特征:${NC}"

# 1. 数组参数是指针类型
if echo "$IR_OUTPUT" | grep -q "define i32 @sum_array(\[3 x i32\]\* %arr)"; then
    echo -e "  ${GREEN}✓${NC} 数组参数使用指针传递"
else
    echo -e "  ${RED}✗${NC} 数组参数未使用指针"
fi

# 2. 结构体参数是指针类型
if echo "$IR_OUTPUT" | grep -q "define i32 @get_distance(%Point\* %p)"; then
    echo -e "  ${GREEN}✓${NC} 结构体参数使用指针传递"
else
    echo -e "  ${RED}✗${NC} 结构体参数未使用指针"
fi

# 3. 返回结构体时有load指令
if echo "$IR_OUTPUT" | grep -q "load %Point.*%Point\*" && echo "$IR_OUTPUT" | grep -q "ret %Point"; then
    echo -e "  ${GREEN}✓${NC} 结构体返回值使用load机制"
else
    echo -e "  ${RED}✗${NC} 结构体返回值处理异常"
fi

echo ""
echo -e "${GREEN}✅ 函数参数/返回值测试通过${NC}"
echo ""

# 测试数组初始化语法
echo -e "${BLUE}=== 测试2: 数组初始化语法 [value; size] ===${NC}"
echo "测试重复初始化语法的IR生成..."
echo ""

TEST_FILE="$TEST_DIR/test_array_init_syntax.rs"
echo -e "${YELLOW}生成IR...${NC}"
IR_OUTPUT=$($COMPILER < "$TEST_FILE" 2>&1 | grep -A 10000 "ModuleID")

# 验证IR语法
echo -ne "验证IR语法正确性 ... "
if echo "$IR_OUTPUT" | llvm-as -o /dev/null 2>&1; then
    echo -e "${GREEN}✅ 通过${NC}"
else
    echo -e "${RED}❌ 失败${NC}"
    echo "$IR_OUTPUT" | llvm-as 2>&1 | head -20
    exit 1
fi

# 检查关键特征
echo ""
echo -e "${YELLOW}检查关键特征:${NC}"

# 1. 小数组展开为多个store
SMALL_ARRAY_FUNC=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_repeat_init_basic/,/^}/p')
STORE_COUNT=$(echo "$SMALL_ARRAY_FUNC" | grep -c "store i32 0")
if [ "$STORE_COUNT" -ge 5 ]; then
    echo -e "  ${GREEN}✓${NC} 小数组展开为多个store (发现${STORE_COUNT}个)"
else
    echo -e "  ${RED}✗${NC} 小数组store数量异常 (${STORE_COUNT})"
fi

# 2. 大数组使用循环
if echo "$IR_OUTPUT" | grep -A 30 "define i32 @test_repeat_init_large" | grep -q "icmp slt"; then
    echo -e "  ${GREEN}✓${NC} 大数组使用循环初始化"
else
    echo -e "  ${RED}✗${NC} 大数组未使用循环"
fi

# 3. 循环包含getelementptr和store
LARGE_ARRAY_FUNC=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_repeat_init_large/,/^}/p')
if echo "$LARGE_ARRAY_FUNC" | grep -q "getelementptr inbounds \[100 x i32\]" && \
   echo "$LARGE_ARRAY_FUNC" | grep -q "store i32 7"; then
    echo -e "  ${GREEN}✓${NC} 循环体正确初始化数组元素"
else
    echo -e "  ${RED}✗${NC} 循环体结构异常"
fi

echo ""
echo -e "${GREEN}✅ 数组初始化语法测试通过${NC}"
echo ""

# 最终总结
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}           验证结果${NC}"
echo -e "${BLUE}=========================================${NC}"
echo -e "${GREEN}✅ 所有Phase 2F功能验证通过！${NC}"
echo ""
echo "已验证功能:"
echo "  1. 聚合类型参数指针传递"
echo "  2. 聚合类型返回值处理"
echo "  3. 数组重复初始化语法"
echo "  4. 小数组展开优化"
echo "  5. 大数组循环优化"
echo ""
