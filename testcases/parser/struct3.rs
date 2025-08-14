fn main() {
    struct Color(u32, u32, u32);
    let c1 = Color(0, 0, 0); // Typical way of creating a tuple struct.
    let c2 = Color {
        0: 255,
        1: 127,
        2: 0,
    }; // Specifying fields by index.
}
