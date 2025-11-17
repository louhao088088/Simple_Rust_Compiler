#!/bin/bash

# 详细分析失败原因
echo "========================================="
echo "  详细失败分析"
echo "========================================="

for i in {1..50}; do
    TEST_DIR="TestCases/IR-1/comprehensive$i"
    RX_FILE="$TEST_DIR/comprehensive$i.rx"
    IN_FILE="$TEST_DIR/comprehensive$i.in"
    
    # IR生成
    timeout 3 ./build/code < "$RX_FILE" > /tmp/comp${i}_full.txt 2>&1
    IR_STATUS=$?
    
    if [ $IR_STATUS -eq 124 ]; then
        echo "comp$i: IR生成超时"
        continue
    elif [ $IR_STATUS -ne 0 ]; then
        echo "comp$i: IR生成失败"
        continue
    fi
    
    # 提取纯IR
    grep -n "^declare" /tmp/comp${i}_full.txt | head -1 | cut -d: -f1 | xargs -I {} tail -n +{} /tmp/comp${i}_full.txt > /tmp/comp${i}.ll
    
    # 检查空条件
    if grep -q "br i1 ," /tmp/comp${i}.ll; then
        echo "comp$i: 空条件表达式"
        continue
    fi
    
    # 运行
    if [ -f "$IN_FILE" ]; then
        timeout 2 lli /tmp/comp${i}.ll < "$IN_FILE" > /tmp/comp${i}_out.txt 2>&1
        RUN_STATUS=$?
        
        if [ $RUN_STATUS -eq 124 ]; then
            echo "comp$i: 运行超时"
        elif [ $RUN_STATUS -ne 0 ]; then
            # 获取具体错误
            ERROR=$(timeout 2 lli /tmp/comp${i}.ll < "$IN_FILE" 2>&1 | grep "error:" | head -1)
            echo "comp$i: 运行错误 - $ERROR"
        else
            echo "comp$i: 运行成功 ✓"
        fi
    fi
done
