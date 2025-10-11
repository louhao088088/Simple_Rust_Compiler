struct Circle {
    radius: u32,
}

impl Circle {
    fn new(r: u32) -> Self {
        Self { radius: r }
    }

    fn area(&self) -> u32 {
        3 * self.radius * self.radius
    }
}

fn main() {
    let c = Circle::new(2);
    exit(0);
}
