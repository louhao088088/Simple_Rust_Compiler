// 测试关键字和标识符的边界情况
fn keywords_and_identifiers() {
    // 关键字作为函数名或变量名的前缀/后缀
    let let_var = 42;
    let if_condition = true;
    let return_value = "test";
    let mut_able = vec![];
    
    // 相似但不同的标识符
    let r#type = "type is keyword";
    let r#match = "match is keyword";
    let r#self = "self is keyword";
    
    // 包含关键字的标识符
    let function_name = "test";
    let struct_data = Data {};
    let enum_variant = Variant::A;
    
    // 生命周期参数
    fn lifetime_test<'a, 'static>(x: &'a str) -> &'a str {
        x
    }
    
    // 原始标识符
    let r#fn = 42;
    let r#struct = "not a struct";
    
    // Self vs self
    impl MyStruct {
        fn new() -> Self {
            Self { field: 0 }
        }
        
        fn method(&self) -> i32 {
            self.field
        }
    }
}
