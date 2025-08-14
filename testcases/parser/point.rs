fn main() {
    let x = 42;
    let r: &i32 = &x;
    let mut y = 5;
    let mr: &mut i32 = &mut y;
    *mr += 1;
}
