#!/bin/bash
echo "详细错误分析"
echo "============================================"

TYPE_MISMATCH=()
UNDEFINED_VALUE=()
UNSIZED_TYPE=()
TIMEOUT=()
SEGFAULT=()
OUTPUT_MISMATCH=()

for i in {1..50}; do
    RX_FILE="TestCases/IR-1/comprehensive$i/comprehensive$i.rx"
    IN_FILE="TestCases/IR-1/comprehensive$i/comprehensive$i.in"
    OUT_FILE="TestCases/IR-1/comprehensive$i/comprehensive$i.out"
    
    timeout 3 ./build/code < "$RX_FILE" 2>&1 | grep -A100000 "^declare" > /tmp/c$i.ll
    result=$(timeout 2 lli /tmp/c$i.ll < "$IN_FILE" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        TIMEOUT+=($i)
    elif [ $exit_code -eq 139 ]; then
        SEGFAULT+=($i)
    elif [ $exit_code -ne 0 ]; then
        if echo "$result" | grep -q "defined with type.*but expected"; then
            TYPE_MISMATCH+=($i)
        elif echo "$result" | grep -q "use of undefined value"; then
            UNDEFINED_VALUE+=($i)
        elif echo "$result" | grep -q "Cannot allocate unsized type"; then
            UNSIZED_TYPE+=($i)
        fi
    else
        if ! diff -q <(echo "$result") "$OUT_FILE" > /dev/null 2>&1; then
            OUTPUT_MISMATCH+=($i)
        fi
    fi
done

echo "类型不匹配 (${#TYPE_MISMATCH[@]}): ${TYPE_MISMATCH[*]}"
echo "未定义值 (${#UNDEFINED_VALUE[@]}): ${UNDEFINED_VALUE[*]}"
echo "无大小类型 (${#UNSIZED_TYPE[@]}): ${UNSIZED_TYPE[*]}"
echo "超时 (${#TIMEOUT[@]}): ${TIMEOUT[*]}"
echo "段错误 (${#SEGFAULT[@]}): ${SEGFAULT[*]}"
echo "输出不匹配 (${#OUTPUT_MISMATCH[@]}): ${OUTPUT_MISMATCH[*]}"
