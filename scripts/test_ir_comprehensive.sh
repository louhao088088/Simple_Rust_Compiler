#!/bin/bash

# IR生成器综合测试脚本
# 测试所有已实现的功能并验证IR正确性

set -e  # 遇到错误立即退出

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

# 编译器路径
COMPILER="./build/code"
TEST_DIR="test1/ir/ir_generator"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}   IR生成器综合测试套件${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""

# 测试函数
test_file() {
    local test_name="$1"
    local test_file="$2"
    local description="$3"
    
    TOTAL=$((TOTAL + 1))
    echo -ne "测试 ${YELLOW}${test_name}${NC}: ${description} ... "
    
    # 生成IR并验证
    if $COMPILER < "$test_file" 2>&1 | grep -A 10000 "ModuleID" | llvm-as -o /dev/null 2>&1; then
        echo -e "${GREEN}✅ PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}❌ FAIL${NC}"
        FAILED=$((FAILED + 1))
        # 显示错误信息
        echo -e "  ${RED}错误信息:${NC}"
        $COMPILER < "$test_file" 2>&1 | grep -A 10000 "ModuleID" | llvm-as 2>&1 | head -10 | sed 's/^/    /'
    fi
}

# ========== Phase 1: 基础功能 ==========
echo -e "${BLUE}=== Phase 1: 基础功能 ===${NC}"
test_file "基础功能v1" "$TEST_DIR/test_basic.rs" "变量、运算、函数调用"
test_file "基础功能v2" "$TEST_DIR/test_basic_v2.rs" "扩展基础功能"
echo ""

# ========== Phase 2A: if表达式 ==========
echo -e "${BLUE}=== Phase 2A: if表达式 ===${NC}"
test_file "if表达式v1" "$TEST_DIR/test_if.rs" "基本if/else"
test_file "if表达式v2" "$TEST_DIR/test_if_v2.rs" "嵌套if和复杂条件"
echo ""

# ========== Phase 2B: 循环 ==========
echo -e "${BLUE}=== Phase 2B: 循环控制流 ===${NC}"
test_file "while循环" "$TEST_DIR/test_while.rs" "while和loop循环"
test_file "循环v2" "$TEST_DIR/test_loops_v2.rs" "循环扩展功能"
echo ""

# ========== Phase 2C: 算法 ==========
echo -e "${BLUE}=== Phase 2C: 综合算法 ===${NC}"
test_file "算法测试" "$TEST_DIR/test_algorithms.rs" "斐波那契、GCD、素数等"
echo ""

# ========== Phase 2D: 数组和结构体 ==========
echo -e "${BLUE}=== Phase 2D: 数组和结构体 ===${NC}"
test_file "数组简单" "$TEST_DIR/test_arrays_simple.rs" "数组字面量、索引、赋值"
test_file "结构体简单" "$TEST_DIR/test_structs_simple.rs" "结构体初始化、字段访问"
echo ""

# ========== Phase 2F: 函数参数与数组初始化 ==========
echo -e "${BLUE}=== Phase 2F: 函数参数与数组初始化 ===${NC}"
test_file "函数参数" "$TEST_DIR/test_function_params.rs" "聚合类型参数/返回值"
test_file "数组初始化" "$TEST_DIR/test_array_init_syntax.rs" "[value; size]语法"
echo ""

# ========== Phase 2G: 多维数组嵌套 ==========
echo -e "${BLUE}=== Phase 2G: 多维数组嵌套 ===${NC}"
test_file "多维数组" "$TEST_DIR/test_nested_arrays.rs" "二维/三维数组、嵌套初始化器"
echo ""

# ========== Phase 2H: impl块和方法 ==========
echo -e "${BLUE}=== Phase 2H: impl块和方法 ===${NC}"
test_file "关联函数" "$TEST_DIR/test_impl_associated_fn.rs" "impl块、Type::function()调用"
test_file "实例方法" "$TEST_DIR/test_impl_methods.rs" "obj.method()调用"
test_file "可变方法" "$TEST_DIR/test_impl_mut_methods.rs" "&mut self方法"
test_file "综合场景" "$TEST_DIR/test_comprehensive.rs" "数组+结构体+方法的综合测试"
test_file "边缘案例" "$TEST_DIR/test_edge_cases.rs" "方法链、嵌套调用、多引用参数"
test_file "复杂场景" "$TEST_DIR/test_complex_scenarios.rs" "结构体数组方法、嵌套结构体、2D数组"
echo ""

# ========== 统计结果 ==========
echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}           测试结果统计${NC}"
echo -e "${BLUE}=========================================${NC}"
echo -e "${GREEN}✅ 通过:${NC} $PASSED"
echo -e "${RED}❌ 失败:${NC} $FAILED"
echo -e "${BLUE}📊 总计:${NC} $TOTAL"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}🎉 所有测试通过！IR生成器工作正常。${NC}"
    exit 0
else
    echo -e "${RED}⚠️  有 $FAILED 个测试失败，请检查错误信息。${NC}"
    exit 1
fi
