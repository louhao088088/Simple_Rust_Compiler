// 测试符号和操作符的边界情况
fn operators_and_symbols() {
    let a = 1;
    let b = 2;
    
    // 算术操作符
    let add = a + b;
    let sub = a - b;
    let mul = a * b;
    let div = a / b;
    let modulo = a % b;
    
    // 比较操作符
    let eq = a == b;
    let ne = a != b;
    let lt = a < b;
    let le = a <= b;
    let gt = a > b;
    let ge = a >= b;
    
    // 逻辑操作符
    let and = true && false;
    let or = true || false;
    let not = !true;
    
    // 位操作符
    let bitand = a & b;
    let bitor = a | b;
    let bitxor = a ^ b;
    let shl = a << 2;
    let shr = a >> 1;
    
    // 赋值操作符
    let mut x = 10;
    x += 5;
    x -= 3;
    x *= 2;
    x /= 4;
    x %= 3;
    x &= 7;
    x |= 1;
    x ^= 2;
    x <<= 1;
    x >>= 1;
    
    // 紧贴的符号测试
    let tight1 = a<<b>>c;
    let tight2 = a<=b>=c;
    let tight3 = a!=b==c;
    
    // 范围操作符
    let range1 = 0..10;
    let range2 = 0..=10;
}
