fn main() {
    let mut a: i32 = 0;
    let arr = [1, 2, 3];
    match &arr {
        [1, 2, 3] => a += 1,
        [1, _, _] => a += 2,
        _ => a += 4,
    }
}
