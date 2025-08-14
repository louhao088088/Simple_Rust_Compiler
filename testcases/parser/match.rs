fn main() {
    let x = 1;

    match x {
        1 => x += 1,
        2 => x += 2,
        3 => x += 3,
        4 => x += 4,
        5 => x += 5,
        _ => x += 8,
    }

    let p = person { name: 1, world: 2 };
}
