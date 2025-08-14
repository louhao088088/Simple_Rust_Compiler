// 测试基本的关键词和标识符
fn basic_keywords_and_identifiers() {
    let mut x = 42;
    let y = true;
    let z = false;
    
    if x > 0 {
        let result = x + y as i32;
    } else {
        let result = x - 1;
    }
    
    while x > 0 {
        x = x - 1;
    }
    
    loop {
        if x == 0 {
            break;
        }
        continue;
    }
    
    match x {
        0 => return,
        1 => x = 2,
        _ => x = 3,
    }
}

struct Point {
    x: i32,
    y: i32,
}

enum Color {
    Red,
    Green,
    Blue,
}

impl Point {
    fn new(x: i32, y: i32) -> Point {
        Point { x, y }
    }
}
