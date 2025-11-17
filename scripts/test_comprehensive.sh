#!/bin/bash
# 综合测试脚本 - 支持末尾换行符差异忽略
# 使用方法: ./test_comprehensive.sh

cd "$(dirname "$0")/../TestCases/IR-1"

echo "=== 开始运行综合测试 (50个测试) ==="
echo "使用 diff -ZB 忽略末尾空白差异"
echo ""

passed=0
failed=0
timeout_count=0
compile_error=0

# 测试结果数组
declare -a pass_tests
declare -a fail_tests
declare -a timeout_tests
declare -a error_tests

for i in {1..50}; do
    printf "测试 comp%-2d: " "$i"
    
    # 生成IR并运行，限时2秒
    if timeout 2s bash -c "
        ../../build/code < comprehensive$i/comprehensive$i.rx 2>&1 | \
        grep -A 99999 '^; ModuleID' > /tmp/c$i.ll 2>&1
    "; then
        # 检查IR是否有效
        if grep -q '^define' /tmp/c$i.ll 2>/dev/null; then
            # 运行并对比输出
            if timeout 2s lli /tmp/c$i.ll < comprehensive$i/comprehensive$i.in 2>&1 > /tmp/c$i.out; then
                # 使用 -ZB 忽略末尾空白和空行差异
                if diff -ZB comprehensive$i/comprehensive$i.out /tmp/c$i.out >/dev/null 2>&1; then
                    echo "✓ PASS"
                    ((passed++))
                    pass_tests+=("$i")
                else
                    echo "✗ 输出不匹配"
                    ((failed++))
                    fail_tests+=("$i")
                fi
            else
                echo "✗ 运行超时或错误"
                ((timeout_count++))
                timeout_tests+=("$i")
            fi
        else
            echo "✗ 编译错误"
            ((compile_error++))
            error_tests+=("$i")
        fi
    else
        echo "✗ 编译超时"
        ((compile_error++))
        error_tests+=("$i")
    fi
done

echo ""
echo "=========================================="
echo "测试结果汇总"
echo "=========================================="
echo "通过: $passed/50 ($((passed*2))%)"
echo "失败: $failed/50"
echo "超时: $timeout_count/50"
echo "编译错误: $compile_error/50"
echo ""

if [ ${#pass_tests[@]} -gt 0 ]; then
    echo "通过的测试:"
    printf "  comp%s\n" "${pass_tests[@]}" | column -c 80
    echo ""
fi

if [ ${#fail_tests[@]} -gt 0 ]; then
    echo "输出不匹配的测试:"
    printf "  comp%s\n" "${fail_tests[@]}" | column -c 80
    echo ""
fi

if [ ${#timeout_tests[@]} -gt 0 ]; then
    echo "超时的测试:"
    printf "  comp%s\n" "${timeout_tests[@]}" | column -c 80
    echo ""
fi

if [ ${#error_tests[@]} -gt 0 ]; then
    echo "编译错误的测试:"
    printf "  comp%s\n" "${error_tests[@]}" | column -c 80
    echo ""
fi

# 返回失败测试数量作为退出码
exit $((50 - passed))
