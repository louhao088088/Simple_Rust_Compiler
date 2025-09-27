/*
Test Package: Semantic-1
Test Target: expr
Author: Wenxin Zheng
Time: 2025-08-08
Verdict: Success
Comment: Hash table with quadratic probing, collision resolution, and load factor analysis; printInt returns `()`
*/

struct HashTableEntry {
    key: i32,
    value: i32,
}

impl HashTableEntry {
    fn set(&mut self) {}
}

fn hello(table: &mut [HashTableEntry; 13]) -> bool {
    let entry: &mut HashTableEntry = &mut table[1 as usize];

    if (true) {
        entry.set();
    }

    return false;
}

fn main() {}
