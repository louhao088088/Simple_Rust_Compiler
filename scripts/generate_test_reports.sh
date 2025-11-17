#!/bin/bash
# IR 测试详细报告生成器

OUTPUT_DIR="test_reports"
mkdir -p "$OUTPUT_DIR"

echo "========================================="
echo "    IR Generator 详细测试报告生成器"
echo "========================================="
echo

# 测试列表
declare -a TESTS=(
    "test1/ir/ir_generator/test_basic.rs:基础功能v1"
    "test1/ir/ir_generator/test_basic_v2.rs:基础功能v2"
    "test1/ir/ir_generator/test_if.rs:if表达式v1"
    "test1/ir/ir_generator/test_if_v2.rs:if表达式v2"
    "test1/ir/ir_generator/test_while.rs:循环v1"
    "test1/ir/ir_generator/test_loops_v2.rs:循环v2"
    "test1/ir/ir_generator/test_algorithms.rs:算法测试"
)

for test_entry in "${TESTS[@]}"; do
    IFS=':' read -r test_file test_name <<< "$test_entry"
    
    echo "处理: $test_name"
    
    # 生成 IR
    output_file="$OUTPUT_DIR/$(basename "$test_file" .rs).ll"
    ./build/code < "$test_file" 2>&1 | awk '/^; ModuleID/,0' > "$output_file"
    
    # 验证 IR
    if llvm-as "$output_file" -o /dev/null 2>&1; then
        echo "  ✅ 验证通过"
        
        # 统计信息
        func_count=$(grep -c "^define" "$output_file")
        block_count=$(grep -c "^[a-z_][a-z0-9_]*:" "$output_file")
        alloca_count=$(grep -c "alloca" "$output_file")
        br_count=$(grep -c "br " "$output_file")
        
        echo "  📊 统计: $func_count 个函数, $block_count 个基本块, $alloca_count 个alloca, $br_count 个分支"
    else
        echo "  ❌ 验证失败"
        llvm-as "$output_file" -o /dev/null 2>&1 | head -3 | sed 's/^/    /'
    fi
    echo
done

echo "========================================="
echo "报告已生成到: $OUTPUT_DIR/"
echo "========================================="
echo

# 生成汇总报告
SUMMARY_FILE="$OUTPUT_DIR/summary.txt"
{
    echo "IR Generator 测试汇总报告"
    echo "生成时间: $(date)"
    echo "========================================"
    echo
    
    for test_entry in "${TESTS[@]}"; do
        IFS=':' read -r test_file test_name <<< "$test_entry"
        output_file="$OUTPUT_DIR/$(basename "$test_file" .rs).ll"
        
        if [ -f "$output_file" ]; then
            echo "[$test_name]"
            echo "  文件: $(basename "$output_file")"
            echo "  大小: $(wc -l < "$output_file") 行"
            
            if llvm-as "$output_file" -o /dev/null 2>&1; then
                echo "  状态: ✅ PASS"
            else
                echo "  状态: ❌ FAIL"
            fi
            echo
        fi
    done
} > "$SUMMARY_FILE"

echo "汇总报告: $SUMMARY_FILE"
cat "$SUMMARY_FILE"
