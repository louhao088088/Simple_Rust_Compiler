#!/bin/bash

# 测试所有comprehensive测试用例
TOTAL=50
PASS=0
FAIL=0

echo "========================================="
echo "  IR生成和运行测试 (使用lli)"
echo "========================================="

for i in $(seq 1 $TOTAL); do
    TEST_DIR="TestCases/IR-1/comprehensive$i"
    RX_FILE="$TEST_DIR/comprehensive$i.rx"
    IN_FILE="$TEST_DIR/comprehensive$i.in"
    OUT_FILE="$TEST_DIR/comprehensive$i.out"
    
    printf "[$i/$TOTAL] comprehensive%-2d ... " $i
    
    # 生成IR
    timeout 3 ./build/code < "$RX_FILE" > /tmp/comp$i.ll 2>&1
    if [ $? -ne 0 ]; then
        echo "FAIL (IR generation timeout/error)"
        ((FAIL++))
        continue
    fi
    
    # 提取纯IR
    grep -n "^declare" /tmp/comp$i.ll | head -1 | cut -d: -f1 | xargs -I {} tail -n +{} /tmp/comp$i.ll > /tmp/comp${i}_clean.ll
    
    # 检查是否有空条件
    if grep -q "br i1 ," /tmp/comp${i}_clean.ll; then
        echo "FAIL (empty condition in IR)"
        ((FAIL++))
        continue
    fi
    
    # 使用lli运行（如果有输入文件）
    if [ -f "$IN_FILE" ]; then
        timeout 2 lli /tmp/comp${i}_clean.ll < "$IN_FILE" > /tmp/comp${i}_output.txt 2>&1
        RUN_STATUS=$?
        if [ $RUN_STATUS -eq 124 ]; then
            echo "TIMEOUT (execution > 2s)"
            ((FAIL++))
            continue
        elif [ $RUN_STATUS -ne 0 ]; then
            echo "FAIL (runtime error)"
            ((FAIL++))
            continue
        fi
        
        # 比较输出
        if [ -f "$OUT_FILE" ]; then
            if diff -q /tmp/comp${i}_output.txt "$OUT_FILE" > /dev/null 2>&1; then
                echo "PASS ✓"
                ((PASS++))
            else
                echo "FAIL (output mismatch)"
                ((FAIL++))
            fi
        else
            echo "PASS (no expected output)"
            ((PASS++))
        fi
    else
        echo "PASS (IR generated)"
        ((PASS++))
    fi
done

echo ""
echo "========================================="
echo "  测试结果"
echo "========================================="
echo "通过: $PASS/$TOTAL"
echo "失败: $FAIL/$TOTAL"
echo "通过率: $(( PASS * 100 / TOTAL ))%"
