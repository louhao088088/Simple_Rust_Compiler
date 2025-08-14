// 测试复杂结构和注释处理
fn complex_structures_and_comments() {
    // 单行注释测试
    let x = 42; // 行末注释
    // 这是一个完整的单行注释
    
    /*
     * 多行注释测试
     * 包含多行内容
     */
    
    let y = /* 行内多行注释 */ 10;
    
    /* 嵌套的 /* 注释 */ 不支持 */
    
    // 复杂的数据结构
    struct ComplexStruct {
        field1: i32,
        field2: bool,
        field3: [i32; 5],
    }
    
    enum ComplexEnum {
        Variant1,
        Variant2(i32),
        Variant3 { x: i32, y: i32 },
    }
    
    impl ComplexStruct {
        fn method1(&self) -> i32 {
            self.field1
        }
        
        fn method2(&mut self, value: i32) {
            self.field1 = value;
        }
    }
    
    // 复杂的函数调用和路径
    let instance = ComplexStruct {
        field1: 1,
        field2: true,
        field3: [1, 2, 3, 4, 5],
    };
    
    let result = instance.method1();
    let array_access = instance.field3[0];
    
    // 复杂的匹配表达式
    match instance.field1 {
        1 => {
            let temp = 100;
            temp + 1
        },
        2 | 3 | 4 => 200,
        _ => 0,
    }
    
    // 模块和可见性
    pub struct PublicStruct;
    pub fn public_function() {}
    
    mod inner_module {
        use super::ComplexStruct;
        
        pub fn module_function() {}
    }
}
