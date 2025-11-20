#!/bin/bash

COMPILER="./build/code"
TEST_DIR="TestCases/IR-1"
TEST_NAME="comprehensive21"
TEST_PATH="$TEST_DIR/$TEST_NAME"
TEMP_LL="/tmp/${TEST_NAME}.ll"

echo "Generating IR for $TEST_NAME..."
cat "$TEST_PATH/${TEST_NAME}.rx" | "$COMPILER" 2>&1 | \
    awk '/^%.*= type|^declare i32 @printf/,0' > "$TEMP_LL"

echo "Checking for 'noalias' attribute..."
if grep -q "noalias" "$TEMP_LL"; then
    echo "✅ 'noalias' attribute found!"
    grep "noalias" "$TEMP_LL" | head -n 5
else
    echo "❌ 'noalias' attribute NOT found!"
fi

echo "Running test with lli (LLVM interpreter)..."
start_time=$(date +%s.%N)
lli "$TEMP_LL" > /dev/null
end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)

echo "Execution time: $duration seconds"
