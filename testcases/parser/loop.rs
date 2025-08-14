fn main() {
    let a = 4;
    let b = 6;

    loop {
        if a > b {
            break;
        } else {
            continue;
        }
        let p = a + b;
    }

    let h = loop {
        if a > b {
            break 2;
        } else {
            continue;
        }
        let p = a + b;
    };
}
