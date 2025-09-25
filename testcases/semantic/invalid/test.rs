/*
Test Package: Semantic-1
Test Target: basic
Author: Wenxin Zheng
Time: 2025-08-08
Verdict: Fail
Comment: basic test, variable shadowing and mutability error
*/

fn main() {
    let index2: usize = 1;
    index2 += 1;
}
