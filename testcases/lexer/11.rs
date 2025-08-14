// 测试字符串和字符字面量
fn string_and_char_literals() {
    // 基本字符串
    let hello = "Hello World";
    let empty = "";
    let single_char = "A";
    
    // 转义字符
    let escaped = "Line 1\nLine 2\tTabbed";
    let quotes = "He said \"Hello\"";
    let backslash = "Path\\to\\file";
    let single_quote = "Don\'t";
    
    // 字符字面量
    let ch1 = 'a';
    let ch2 = 'Z';
    let ch3 = '0';
    let ch4 = ' ';
    
    // 转义字符字面量
    let newline_char = '\n';
    let tab_char = '\t';
    let quote_char = '\"';
    let backslash_char = '\\';
    let single_quote_char = '\'';
    
    // 特殊ASCII字符
    let null_char = '\0';
    let carriage_return = '\r';
    
    // 测试边界情况
    let very_long_string = "This is a very long string that contains many characters to test the lexer with longer input sequences";
}
