/*
Test Package: Semantic-1
Test Target: misc
Author: Wenxin Zheng
Time: 2025-08-08
Verdict: Success
Comment: misc test, simple calculator with enums
*/

// Simple calculator with basic operations
// Perform arithmetic operations on two numbers
enum Operation {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
}

fn perform_operation(a: i32, b: i32, op: Operation) -> i32 {
    if (op == Operation::Add) {
        return a + b;
    }
    1
}

fn main() {
    exit(0);
}
