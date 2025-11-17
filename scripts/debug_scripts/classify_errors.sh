#!/bin/bash
echo "========================================="
echo "  失败分类统计"
echo "========================================="

success=0
timeout=0
entry_error=0
type_mismatch=0
undefined_value=0
unsized_type=0
segfault=0
other_error=0

for i in {1..50}; do
    IN_FILE="TestCases/semantic-2/comp${i}.in"
    EXPECTED_FILE="TestCases/semantic-2/comp${i}.out"
    
    ./build/code < "TestCases/semantic-2/comp${i}.rx" 2>&1 | grep -A100000 "^declare" > /tmp/comp${i}.ll
    result=$(timeout 2 lli /tmp/comp${i}.ll < "$IN_FILE" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        if diff -q <(echo "$result") "$EXPECTED_FILE" > /dev/null 2>&1; then
            ((success++))
        else
            ((other_error++))
            echo "comp${i}: 输出不匹配"
        fi
    elif [ $exit_code -eq 124 ]; then
        ((timeout++))
        echo "comp${i}: 超时"
    elif [ $exit_code -eq 139 ]; then
        ((segfault++))
        echo "comp${i}: 段错误"
    else
        if echo "$result" | grep -q "unable to create block named 'entry'"; then
            ((entry_error++))
            echo "comp${i}: entry块错误"
        elif echo "$result" | grep -q "defined with type.*but expected"; then
            ((type_mismatch++))
            echo "comp${i}: 类型不匹配 - $(echo "$result" | grep "defined with type" | head -1)"
        elif echo "$result" | grep -q "use of undefined value"; then
            ((undefined_value++))
            echo "comp${i}: 未定义值 - $(echo "$result" | grep "undefined value" | head -1)"
        elif echo "$result" | grep -q "Cannot allocate unsized type"; then
            ((unsized_type++))
            echo "comp${i}: 无大小类型"
        else
            ((other_error++))
            echo "comp${i}: 其他错误 - $result"
        fi
    fi
done

echo ""
echo "========================================="
echo "统计："
echo "  成功: $success/50"
echo "  超时: $timeout"
echo "  entry块错误: $entry_error"
echo "  类型不匹配: $type_mismatch"
echo "  未定义值: $undefined_value"
echo "  无大小类型: $unsized_type"
echo "  段错误: $segfault"
echo "  其他错误: $other_error"
echo "========================================="
