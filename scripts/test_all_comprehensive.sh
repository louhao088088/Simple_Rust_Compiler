#!/bin/bash

# 综合测试脚本 - 测试所有comprehensive测试用例
# 忽略末尾换行符差异
# 使用并行编译和测试以提高速度

COMPILER="/home/louhao/compiler/build/code"
TEST_DIR="/home/louhao/compiler/TestCases/IR-1"
TEMP_DIR="/tmp/compiler_test"
TIMEOUT=30  # 增加超时时间以应对大型测试
MAX_PARALLEL=8  # 最大并行数

mkdir -p "$TEMP_DIR"

passed=0
failed=0
timeout_count=0

echo "========================================"
echo "开始测试所有comprehensive测试用例"
echo "========================================"
echo ""

# 第一阶段：并行编译所有IR文件
echo "阶段1: 并行编译IR文件..."
compile_count=0
for i in {1..50}; do
    test_name="comprehensive$i"
    test_path="$TEST_DIR/$test_name"
    
    if [ ! -d "$test_path" ]; then
        continue
    fi
    
    # 后台并行编译
    (
        cat "$test_path/${test_name}.rx" | "$COMPILER" 2>&1 | \
            awk '/^%.*= type|^declare i32 @printf/,0' > "$TEMP_DIR/${test_name}.ll" 2>&1
    ) &
    
    ((compile_count++))
    
    # 控制并行数量
    if [ $((compile_count % MAX_PARALLEL)) -eq 0 ]; then
        wait
    fi
done
wait  # 等待所有编译完成
echo "编译完成！"
echo ""

# 第二阶段：测试每个用例（对于快速测试可并行，慢速测试串行）
for i in {1..50}; do
    test_name="comprehensive$i"
    test_path="$TEST_DIR/$test_name"
    
    if [ ! -d "$test_path" ]; then
        continue
    fi
    
    printf "测试 %-18s ... " "$test_name"
    
    # 记录开始时间
    start_time=$(date +%s.%N)
    
    # 检查编译是否成功
    if [ ! -s "$TEMP_DIR/${test_name}.ll" ]; then
        elapsed=$(date +%s.%N | awk -v start=$start_time '{printf "%.2f", $1-start}')
        echo "❌ IR生成失败 (${elapsed}s)"
        ((failed++))
        continue
    fi
    
    # 先用 opt 优化 IR（仅 mem2reg，提升局部变量到寄存器）
    # 对于大型 IR，避免使用 -O1 等重量级优化（会导致内存溢出）
    opt -mem2reg "$TEMP_DIR/${test_name}.ll" -o "$TEMP_DIR/${test_name}_opt.bc" 2>/dev/null
    
    # 如果 opt 失败（可能内存不足），回退到原始 IR
    if [ ! -f "$TEMP_DIR/${test_name}_opt.bc" ] || [ ! -s "$TEMP_DIR/${test_name}_opt.bc" ]; then
        cp "$TEMP_DIR/${test_name}.ll" "$TEMP_DIR/${test_name}_opt.bc"
    fi
    
    # 运行（带超时）
    if [ -f "$test_path/${test_name}.in" ]; then
        timeout $TIMEOUT lli "$TEMP_DIR/${test_name}_opt.bc" < "$test_path/${test_name}.in" > "$TEMP_DIR/${test_name}.out" 2>&1
    else
        timeout $TIMEOUT lli "$TEMP_DIR/${test_name}_opt.bc" > "$TEMP_DIR/${test_name}.out" 2>&1
    fi
    
    exit_code=$?
    elapsed=$(date +%s.%N | awk -v start=$start_time '{printf "%.2f", $1-start}')
    
    if [ $exit_code -eq 124 ]; then
        echo "⏱️  超时 (>${TIMEOUT}s)"
        ((timeout_count++))
    elif [ $exit_code -ne 0 ]; then
        echo "❌ 运行时错误 (退出码: $exit_code, ${elapsed}s)"
        ((failed++))
    elif diff -Z "$test_path/${test_name}.out" "$TEMP_DIR/${test_name}.out" > /dev/null 2>&1; then
        echo "✅ 通过 (${elapsed}s)"
        ((passed++))
    else
        diff_lines=$(diff -Z "$test_path/${test_name}.out" "$TEMP_DIR/${test_name}.out" 2>/dev/null | grep -c "^<\|^>")
        echo "❌ 输出不匹配 ($diff_lines 处差异, ${elapsed}s)"
        ((failed++))
    fi
done

echo ""
echo "========================================"
echo "测试结果汇总"
echo "========================================"
echo "通过:   $passed"
echo "失败:   $failed"
echo "超时:   $timeout_count"
total=$((passed + failed + timeout_count))
echo "总计:   $total"
if [ $total -gt 0 ]; then
    percentage=$((passed * 100 / total))
    echo "通过率: $percentage%"
fi
echo "========================================"

# 返回失败测试数量作为退出码
exit $failed
