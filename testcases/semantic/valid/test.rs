/*
Test Package: Semantic-1
Test Target: basic
Author: Wenxin Zheng
Time: 2025-08-08
Verdict: Success
Comment: basic test, quickselect algorithm for finding median
*/

fn select_k(a: &mut [i32; 11], low: usize, high: usize, k: usize) -> i32 {
    if (low == high) {
        return a[low];
    }
    let p: usize = 1;
    if (k == p) {
        a[p]
    } else if (k < p) {
        select_k(a, low, p - 1, k)
    } else {
        select_k(a, p + 1, high, k)
    }
}
fn main() {}
