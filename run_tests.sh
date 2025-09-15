#!/bin/bash

# =======================================================
#               请确认以下配置
# =======================================================
COMPILER_EXEC="./build/code"
VALID_TEST_DIR="./testcases/semantic/valid"
INVALID_TEST_DIR="./testcases/semantic/invalid"
# =======================================================

# 初始化计数器
passed_count=0
failed_count=0
total_count=0
failed_tests=""

# --- 函数定义 ---
print_success() {
    echo -e "\e[32m$1\e[0m"
}
print_failure() {
    echo -e "\e[31m$1\e[0m"
}

echo "========================================="
echo "  Running VALID compilation tests...     "
echo "  (Directory: $VALID_TEST_DIR)          "
echo "========================================="

for test_file in $(find "$VALID_TEST_DIR" -name "*.rs"); do
    ((total_count++))
    echo "Testing $test_file ... (should pass)"
    # 注意这里的 '<' 符号
    if "$COMPILER_EXEC" < "$test_file" >/dev/null 2>&1; then
        print_success "  PASSED"
        ((passed_count++))
    else
        print_failure "  FAILED: Expected to compile, but it failed."
        ((failed_count++))
        failed_tests+="$test_file\n"
    fi
done

echo ""
echo "========================================="
echo "  Running INVALID compilation tests...   "
echo "  (Directory: $INVALID_TEST_DIR)        "
echo "========================================="

for test_file in $(find "$INVALID_TEST_DIR" -name "*.rs"); do
    ((total_count++))
    echo "Testing $test_file ... (should fail)"
    # 注意这里的 '<' 符号
    if ! "$COMPILER_EXEC" < "$test_file" >/dev/null 2>&1; then
        print_success "  PASSED"
        ((passed_count++))
    else
        print_failure "  FAILED: Expected a compilation error, but it succeeded."
        ((failed_count++))
        failed_tests+="$test_file\n"
    fi
done

echo ""
echo "========================================="
echo "              Test Summary               "
echo "========================================="
echo "Total tests: $total_count"
print_success "Passed: $passed_count"
print_failure "Failed: $failed_count"
echo "========================================="

if [ $failed_count -gt 0 ]; then
    echo ""
    print_failure "List of failed tests:"
    echo -e "$failed_tests"
    exit 1
else
    print_success "All tests passed successfully!"
    exit 0
fi