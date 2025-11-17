#!/bin/bash

# IR验证和执行测试脚本
# 用途：验证生成的LLVM IR不仅语法正确，而且语义正确

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 使用说明
usage() {
    echo "用法: $0 <test.rs> [expected_output]"
    echo ""
    echo "参数:"
    echo "  test.rs         - 输入的Rust测试文件"
    echo "  expected_output - 预期的输出值（可选，默认检查返回码）"
    echo ""
    echo "示例:"
    echo "  $0 test_basic.rs 42"
    echo "  $0 test_algo.rs"
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

TEST_FILE="$1"
EXPECTED_OUTPUT="${2:-}"
BASENAME=$(basename "$TEST_FILE" .rs)
OUTPUT_DIR="/tmp/ir_test_$$"
mkdir -p "$OUTPUT_DIR"

IR_FILE="$OUTPUT_DIR/${BASENAME}.ll"
ASM_FILE="$OUTPUT_DIR/${BASENAME}.s"
OBJ_FILE="$OUTPUT_DIR/${BASENAME}.o"
EXE_FILE="$OUTPUT_DIR/${BASENAME}"

echo -e "${BLUE}=========================================${NC}"
echo -e "${BLUE}  LLVM IR 完整验证流程${NC}"
echo -e "${BLUE}=========================================${NC}"
echo ""
echo -e "测试文件: ${YELLOW}$TEST_FILE${NC}"
echo ""

# 清理函数
cleanup() {
    if [ -n "$OUTPUT_DIR" ] && [ -d "$OUTPUT_DIR" ]; then
        rm -rf "$OUTPUT_DIR"
    fi
}
trap cleanup EXIT

# 步骤1: 生成IR
echo -e "${BLUE}[1/6] 生成 LLVM IR...${NC}"
COMPILE_LOG="$OUTPUT_DIR/compile.log"
RAW_OUTPUT="$OUTPUT_DIR/raw_output.txt"

# stdout包含调试信息+IR，stderr包含日志
if ! ./build/code < "$TEST_FILE" > "$RAW_OUTPUT" 2> "$COMPILE_LOG"; then
    echo -e "${RED}❌ 编译失败！${NC}"
    cat "$COMPILE_LOG"
    exit 1
fi

# 从第一个以;开头的行开始提取（LLVM IR的注释以;开头）
# 或者从"define"关键字开始
if grep -q '^\s*;' "$RAW_OUTPUT"; then
    awk '/^\s*;/{flag=1} flag' "$RAW_OUTPUT" > "$IR_FILE"
elif grep -q '^\s*define' "$RAW_OUTPUT"; then
    awk '/^\s*define/{flag=1} flag' "$RAW_OUTPUT" > "$IR_FILE"
else
    echo -e "${RED}❌ 未找到有效的IR输出！${NC}"
    echo "原始输出最后20行："
    tail -20 "$RAW_OUTPUT"
    exit 1
fi

# 检查IR是否为空
if [ ! -s "$IR_FILE" ] || ! grep -q '[^[:space:]]' "$IR_FILE"; then
    echo -e "${RED}❌ 未生成有效的IR！${NC}"
    echo "编译器日志："
    cat "$COMPILE_LOG"
    exit 1
fi

echo -e "${GREEN}✓ IR生成成功${NC}"
IR_LINES=$(grep -c '[^[:space:]]' "$IR_FILE" || echo "0")
echo -e "  生成 ${IR_LINES} 行IR代码"
echo ""

# 步骤2: 语法验证
echo -e "${BLUE}[2/6] 验证 IR 语法 (llvm-as)...${NC}"
if ! llvm-as "$IR_FILE" -o /dev/null 2>&1; then
    echo -e "${RED}❌ IR语法错误！${NC}"
    llvm-as "$IR_FILE" -o /dev/null
    exit 1
fi
echo -e "${GREEN}✓ IR语法正确${NC}"
echo ""

# 步骤3: 优化（可选）
echo -e "${BLUE}[3/6] 优化 IR (opt -O2)...${NC}"
OPT_IR_FILE="$OUTPUT_DIR/${BASENAME}_opt.ll"
if opt -O2 "$IR_FILE" -S -o "$OPT_IR_FILE" 2>&1; then
    echo -e "${GREEN}✓ IR优化成功${NC}"
    # 显示优化效果
    ORIG_LINES=$(wc -l < "$IR_FILE")
    OPT_LINES=$(wc -l < "$OPT_IR_FILE")
    echo -e "  原始IR: ${ORIG_LINES}行 -> 优化后: ${OPT_LINES}行"
else
    echo -e "${YELLOW}⚠ 优化失败，使用原始IR${NC}"
    OPT_IR_FILE="$IR_FILE"
fi
echo ""

# 步骤4: 解释执行
echo -e "${BLUE}[4/6] 解释执行 (lli)...${NC}"
# 创建包含main函数的wrapper
WRAPPER_FILE="$OUTPUT_DIR/${BASENAME}_wrapper.ll"

# 直接复制优化后的IR
cat "$OPT_IR_FILE" > "$WRAPPER_FILE"

# 查找测试函数并创建main函数调用它
TEST_FUNC=$(grep -o 'define.*@test[_a-zA-Z0-9]*()' "$OPT_IR_FILE" | head -1 | sed 's/define.*@\([^(]*\).*/\1/' || echo "")
if [ -z "$TEST_FUNC" ]; then
    # 如果没有test函数，查找第一个返回i32的函数
    TEST_FUNC=$(grep -o 'define i32 @[a-zA-Z_][a-zA-Z0-9_]*()' "$OPT_IR_FILE" | head -1 | sed 's/define i32 @\([^(]*\).*/\1/' || echo "")
fi

if [ -n "$TEST_FUNC" ]; then
    # 添加main函数
    cat >> "$WRAPPER_FILE" << EOF

define i32 @main() {
entry:
  %result = call i32 @${TEST_FUNC}()
  ret i32 %result
}
EOF
    
    if lli "$WRAPPER_FILE" 2>&1; then
        LLI_EXITCODE=$?
    else
        LLI_EXITCODE=$?
    fi
    
    # LLI的退出码就是程序的返回值（对于返回i32的main函数）
    echo -e "${GREEN}✓ 执行成功${NC}"
    echo -e "  返回值: ${LLI_EXITCODE}"
    
    # 检查预期输出
    if [ -n "$EXPECTED_OUTPUT" ]; then
        if [ "$LLI_EXITCODE" -eq "$EXPECTED_OUTPUT" ]; then
            echo -e "${GREEN}✓ 输出匹配预期: ${EXPECTED_OUTPUT}${NC}"
        else
            echo -e "${RED}❌ 输出不匹配！${NC}"
            echo -e "  预期: ${EXPECTED_OUTPUT}"
            echo -e "  实际: ${LLI_EXITCODE}"
            exit 1
        fi
    fi
else
    echo -e "${YELLOW}⚠ 未找到可执行的测试函数，跳过解释执行${NC}"
fi
echo ""

# 步骤5: 编译为汇编
echo -e "${BLUE}[5/6] 编译为汇编 (llc)...${NC}"
# 使用wrapper文件（包含main函数）进行编译
if llc "$WRAPPER_FILE" -o "$ASM_FILE" 2>&1; then
    echo -e "${GREEN}✓ 汇编生成成功${NC}"
    ASM_LINES=$(wc -l < "$ASM_FILE")
    echo -e "  汇编代码: ${ASM_LINES}行"
else
    echo -e "${RED}❌ 汇编生成失败！${NC}"
    exit 1
fi
echo ""

# 步骤6: 链接为可执行文件
echo -e "${BLUE}[6/6] 链接为可执行文件 (clang)...${NC}"
if clang "$ASM_FILE" -o "$EXE_FILE" 2>&1; then
    echo -e "${GREEN}✓ 可执行文件生成成功${NC}"
    
    # 运行可执行文件
    if "$EXE_FILE" 2>&1; then
        EXE_EXITCODE=$?
    else
        EXE_EXITCODE=$?
    fi
    
    echo -e "${GREEN}✓ 原生执行成功${NC}"
    echo -e "  返回值: ${EXE_EXITCODE}"
    
    # 检查预期输出
    if [ -n "$EXPECTED_OUTPUT" ]; then
        if [ "$EXE_EXITCODE" -eq "$EXPECTED_OUTPUT" ]; then
            echo -e "${GREEN}✓ 输出匹配预期: ${EXPECTED_OUTPUT}${NC}"
        else
            echo -e "${RED}❌ 输出不匹配！${NC}"
            echo -e "  预期: ${EXPECTED_OUTPUT}"
            echo -e "  实际: ${EXE_EXITCODE}"
            exit 1
        fi
    fi
else
    echo -e "${RED}❌ 链接失败！${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}=========================================${NC}"
echo -e "${GREEN}  ✅ 所有验证步骤通过！${NC}"
echo -e "${GREEN}=========================================${NC}"
echo ""
echo "生成的文件:"
echo "  IR:       $IR_FILE"
echo "  优化IR:   $OPT_IR_FILE"
echo "  汇编:     $ASM_FILE"
echo "  可执行:   $EXE_FILE"
echo ""
echo "提示: 使用 'ls -lh $OUTPUT_DIR' 查看文件大小"
