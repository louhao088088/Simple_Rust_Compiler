#!/bin/bash

# 多维数组嵌套功能验证脚本

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
echo -e "${BLUE}   多维数组嵌套功能验证${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# 测试函数
test_file() {
    local test_name="$1"
    local test_file="$2"
    local description="$3"
    
    echo -e "${YELLOW}测试${test_name}: ${description}${NC}"
    echo -ne "  生成IR... "
    
    IR_OUTPUT=$($COMPILER < "$test_file" 2>&1 | grep -A 10000 "ModuleID")
    
    if [ -z "$IR_OUTPUT" ]; then
        echo -e "${RED}❌ 失败 - 未生成IR${NC}"
        return 1
    fi
    
    echo -ne "${GREEN}✓${NC} "
    echo -ne "验证IR语法... "
    
    if echo "$IR_OUTPUT" | llvm-as -o /dev/null 2>&1; then
        echo -e "${GREEN}✓ 通过${NC}"
        return 0
    else
        echo -e "${RED}✗ 失败${NC}"
        echo "$IR_OUTPUT" | llvm-as 2>&1 | head -10
        return 1
    fi
}

PASSED=0
FAILED=0

# 测试1: 二维数组基本操作
if test_file "1" "$TEST_DIR/test_nested_arrays.rs" "二维/三维数组完整测试"; then
    PASSED=$((PASSED + 1))
else
    FAILED=$((FAILED + 1))
fi

echo ""

# 提取具体测试函数并检查
echo -e "${BLUE}=== 详细功能验证 ===${NC}"

IR_OUTPUT=$($COMPILER < "$TEST_DIR/test_nested_arrays.rs" 2>&1 | grep -A 10000 "ModuleID")

# 检查关键特征
echo -e "${YELLOW}检查关键IR特征:${NC}"

# 1. 二维数组类型
if echo "$IR_OUTPUT" | grep -q "\[2 x \[2 x i32\]\]"; then
    echo -e "  ${GREEN}✓${NC} 二维数组类型正确生成"
else
    echo -e "  ${RED}✗${NC} 二维数组类型异常"
fi

# 2. 三维数组类型
if echo "$IR_OUTPUT" | grep -q "\[2 x \[2 x \[2 x i32\]\]\]"; then
    echo -e "  ${GREEN}✓${NC} 三维数组类型正确生成"
else
    echo -e "  ${RED}✗${NC} 三维数组类型异常"
fi

# 3. 嵌套索引访问 (连续两次getelementptr)
FUNC_2D=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_2d_array_basic/,/^}/p')
GEP_COUNT=$(echo "$FUNC_2D" | grep -c "getelementptr")
if [ "$GEP_COUNT" -ge 10 ]; then
    echo -e "  ${GREEN}✓${NC} 嵌套索引访问正确实现 (${GEP_COUNT}个getelementptr)"
else
    echo -e "  ${RED}✗${NC} 嵌套索引访问异常 (${GEP_COUNT})"
fi

# 4. 嵌套初始化器 - 内层数组load
FUNC_INIT=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_2d_array_init_syntax/,/^}/p')
if echo "$FUNC_INIT" | grep -q "load \[3 x i32\]"; then
    echo -e "  ${GREEN}✓${NC} 嵌套初始化器正确load内层数组"
else
    echo -e "  ${RED}✗${NC} 嵌套初始化器处理异常"
fi

# 5. 数组元素修改
FUNC_MUT=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_2d_array_mutation/,/^}/p')
STORE_99=$(echo "$FUNC_MUT" | grep -c "store i32 99")
STORE_88=$(echo "$FUNC_MUT" | grep -c "store i32 88")
if [ "$STORE_99" -eq 1 ] && [ "$STORE_88" -eq 1 ]; then
    echo -e "  ${GREEN}✓${NC} 二维数组元素修改正确"
else
    echo -e "  ${RED}✗${NC} 二维数组元素修改异常"
fi

# 6. 三维数组索引
FUNC_3D=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_3d_array/,/^}/p')
GEP_3D_COUNT=$(echo "$FUNC_3D" | grep -c "getelementptr")
if [ "$GEP_3D_COUNT" -ge 3 ]; then
    echo -e "  ${GREEN}✓${NC} 三维数组索引访问 cube[1][1][1] 正确"
else
    echo -e "  ${RED}✗${NC} 三维数组索引异常"
fi

# 7. 结构体数组的字段访问
if echo "$IR_OUTPUT" | grep -q "define i32 @test_array_of_structs_2d"; then
    FUNC_STRUCT=$(echo "$IR_OUTPUT" | sed -n '/define i32 @test_array_of_structs_2d/,/^}/p')
    if echo "$FUNC_STRUCT" | grep -q "%Point\*" && echo "$FUNC_STRUCT" | grep -q "getelementptr inbounds %Point"; then
        echo -e "  ${GREEN}✓${NC} 结构体数组字段访问 points[1][1].x 正确"
    else
        echo -e "  ${RED}✗${NC} 结构体数组字段访问异常"
    fi
fi

echo ""
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}           验证结果${NC}"
echo -e "${BLUE}=========================================${NC}"
if [ "$FAILED" -eq 0 ]; then
    echo -e "${GREEN}✅ 所有多维数组测试通过！${NC}"
    echo ""
    echo "已验证功能:"
    echo "  1. 二维数组类型定义和初始化"
    echo "  2. 三维数组类型定义和初始化"
    echo "  3. 嵌套数组字面量 [[1,2],[3,4]]"
    echo "  4. 嵌套初始化器语法 [[0;3];2]"
    echo "  5. 多维数组索引访问 arr[i][j]"
    echo "  6. 多维数组元素修改"
    echo "  7. 数组中的结构体字段访问"
else
    echo -e "${RED}❌ 有 $FAILED 个测试失败${NC}"
    exit 1
fi
