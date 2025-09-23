/*
Test Package: Semantic-1
Test Target: return
Author: Wenxin Zheng
Time: 2025-08-07
Verdict: Success
Comment: exit function test, only `i32` can be returned
*/

fn main() {
    let b: u32 = 3 + 1;

    let c: String = b.to_string();
}
