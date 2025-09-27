fn main() {
    let a: Point = Point { x: 3, y: 4 };
    let b: i32 = a.hello();
}

struct Point {
    x: i32,
    y: i32,
}

impl Point {
    fn hello(&self) -> i32 {
        1
    }
}
