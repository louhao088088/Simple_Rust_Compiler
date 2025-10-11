fn main() {
    enum Foo {
        Bar,       // 0
        Baz = 123, // 123
        Quux,      // 124
    }
    exit(0);
}
enum Animal {
    Dog,
    Cat,
}

enum Examples {
    UnitLike,
    TupleLike(i32),
    StructLike { value: i32 },
}
