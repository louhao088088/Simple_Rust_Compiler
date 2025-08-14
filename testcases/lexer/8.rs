// 测试复杂的符号组合和边界情况
fn symbols_and_edge_cases() {
    // 连续符号的区分
    let x = 1<< 2>>3;
    let y = a<<b>>c;
    let z = x<<= y>>=z;
    
    // 箭头和比较运算符
    let func = |x| -> i32 { x + 1 };
    let result = match value {
        x if x >= 10 => "big",
        x if x <= 0 => "small",
        _ => "medium",
    };
    
    // 路径分隔符
    use std::collections::HashMap;
    let path = crate::module::function();
    let associated = Type::CONSTANT;
    
    // 点操作符的不同用法
    let chained = obj.method().field.another_method();
    let range = 0..10;
    let inclusive = 0..=10;
    let from = 5..;
    let to = ..10;
    let full = ..;
    
    // 复杂的泛型和生命周期
    let map: HashMap<String, Vec<Option<&'static str>>> = HashMap::new();
    
    // 宏调用
    println!("Hello, {}!", name);
    vec![1, 2, 3];
    
    // 属性
    #[derive(Debug, Clone)]
    #[cfg(feature = "special")]
    struct Test;
    
    // 问号操作符
    let result = operation()?;
    let optional = value?.field?.method()?;
}
