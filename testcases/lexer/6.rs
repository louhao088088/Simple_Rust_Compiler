// 测试注释和空白字符的复杂情况
fn comments_and_whitespace() {
    /* 多行注释
       可以跨越多行
       /* 甚至可以嵌套 */
       继续注释 */
    let x = 42; // 行末注释
    
    // 混合在代码中的注释
    let y = /* 行内注释 */ 10 + /* 另一个注释 */ 20;
    
    /* 注释中的特殊字符: "string" 'c' \n \t */
    let z = 5; /* 注释紧贴代码 */let w = 6;
    
    // 特殊情况：注释符号在字符串中
    let not_comment = "this // is not a comment";
    let also_not = "/* this is not a comment */";
    
    // URL和路径可能包含 //
    let url = "https://example.com/path";
    
    /*
     * 带星号装饰的注释块
     * 每行都有星号
     */
}
