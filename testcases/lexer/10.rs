// 测试各种数字字面量格式
fn number_literals_test() {
    // 十进制整数
    let decimal = 123;
    let with_underscore = 1_000_000;
    let large_number = 999999999;
    
    // 十六进制
    let hex1 = 0xFF;
    let hex2 = 0xDEADBEEF;
    let hex3 = 0x123abc;
    
    // 二进制
    let binary1 = 0b1010;
    let binary2 = 0b11111111;
    let binary3 = 0b0000_1111;
    
    // 八进制
    let octal1 = 0o777;
    let octal2 = 0o123;
    
    // 边界测试
    let zero = 0;
    let one = 1;
    let max_val = 4294967295;
    
    // 类型后缀
    let typed_int = 42i32;
    let typed_uint = 42u64;
    let byte_val = 255u8;
}
