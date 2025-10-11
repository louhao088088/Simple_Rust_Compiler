fn main() {
    let a = 4;
    let b = 6;

    while a < b {
        if a > b {
            break;
        } else {
            continue;
        }
        let p = a + b;
    }

    let h = while a < b {
        if a > b {
            break;
        } else {
            continue;
        }
        let p = a + b;
    };
    exit(0);
}
