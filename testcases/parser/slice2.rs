fn main() {
    let arr = [10, 20, 30, 40];

    let s1: &[i32] = &arr;
    let s2: &[i32] = &arr[1..];
    let s3: &mut [i32] = &mut arr[2..];
    s3[0] = 99;
    exit(0);
}
