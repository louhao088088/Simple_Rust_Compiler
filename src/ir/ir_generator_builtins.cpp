/**
 * ir_generator_builtins.cpp
 *
 * 内置函数支持模块
 *
 * 职责：
 * - 声明C标准库函数（printf, scanf, exit）
 * - 定义格式化字符串常量
 * - 实现内置I/O函数（printInt, printlnInt, getInt, exit）
 *
 * 这个文件实现了编译器提供的内置函数，它们实际上是对C标准库
 * 函数的封装。包括：
 * 1. printInt/printlnInt - 整数输出（使用printf）
 * 2. getInt - 整数输入（使用scanf）
 * 3. exit - 程序退出
 *
 * 注意：这些函数使用vararg调用约定，需要特殊的IR语法。
 */

#include "ir_generator.h"

// ========== 内置函数声明 ==========

void IRGenerator::emit_builtin_declarations() {
    // 声明 printf (用于 printInt, printlnInt)
    // declare i32 @printf(i8*, ...)
    emitter_.emit_function_declaration("i32", "printf", {"i8*"}, true);

    // 声明 scanf (用于 getInt)
    // declare i32 @scanf(i8*, ...)
    emitter_.emit_function_declaration("i32", "scanf", {"i8*"}, true);

    // 声明 exit (用于 exit)
    // declare void @exit(i32)
    emitter_.emit_function_declaration("void", "exit", {"i32"}, false);

    // 声明 llvm.memset.p0.i64 (用于大数组零初始化优化)
    // declare void @llvm.memset.p0.i64(i8*, i8, i64, i1)
    emitter_.emit_function_declaration("void", "llvm.memset.p0.i64", {"i8*", "i8", "i64", "i1"},
                                       false);

    // 声明 llvm.memcpy.p0.p0.i64 (用于数组/结构体拷贝)
    // declare void @llvm.memcpy.p0.p0.i64(i8*, i8*, i64, i1)
    emitter_.emit_function_declaration("void", "llvm.memcpy.p0.p0.i64", {"i8*", "i8*", "i64", "i1"},
                                       false);

    emitter_.emit_blank_line();

    // 定义格式化字符串常量
    // @.str.int = private unnamed_addr constant [3 x i8] c"%d\00"
    emitter_.emit_global_variable(".str.int", "[3 x i8]", "c\"%d\\00\"", true);

    // @.str.int_newline = private unnamed_addr constant [4 x i8] c"%d\0A\00"
    emitter_.emit_global_variable(".str.int_newline", "[4 x i8]", "c\"%d\\0A\\00\"", true);

    // @.str.int_scanf = private unnamed_addr constant [3 x i8] c"%d\00"
    emitter_.emit_global_variable(".str.int_scanf", "[3 x i8]", "c\"%d\\00\"", true);

    emitter_.emit_blank_line();
}

// ========== 内置函数实现 ==========

bool IRGenerator::handle_builtin_function(
    CallExpr *node, const std::string &func_name,
    const std::vector<std::pair<std::string, std::string>> &args) {
    // printInt(n: i32) -> ()
    // 输出整数（不换行）
    if (func_name == "printInt") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        // 获取格式化字符串的指针
        // getelementptr [3 x i8], [3 x i8]* @.str.int, i32 0, i32 0
        std::string fmt_ptr =
            emitter_.emit_getelementptr("[3 x i8]", "@.str.int", {"i32 0", "i32 0"});

        // 调用 printf (vararg函数)
        // %0 = call i32 (i8*, ...) @printf(i8* %fmt, i32 %arg)
        emitter_.emit_vararg_call("i32", "printf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32", arg_value}});

        store_expr_result(node, "");
        return true;
    }

    // printlnInt(n: i32) -> ()
    // 输出整数（带换行）
    if (func_name == "printlnInt") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        // 使用带换行符的格式化字符串
        std::string fmt_ptr =
            emitter_.emit_getelementptr("[4 x i8]", "@.str.int_newline", {"i32 0", "i32 0"});

        emitter_.emit_vararg_call("i32", "printf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32", arg_value}});

        store_expr_result(node, "");
        return true;
    }

    // getInt() -> i32
    // 从标准输入读取整数
    if (func_name == "getInt") {
        if (args.size() != 0) {
            return false;
        }

        // 分配临时空间存储读取的值
        // %temp = alloca i32
        std::string temp_var = emitter_.emit_alloca("i32");

        // 获取格式化字符串
        std::string fmt_ptr =
            emitter_.emit_getelementptr("[3 x i8]", "@.str.int_scanf", {"i32 0", "i32 0"});

        // 调用 scanf
        // %0 = call i32 (i8*, ...) @scanf(i8* %fmt, i32* %temp)
        emitter_.emit_vararg_call("i32", "scanf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32*", temp_var}});

        // 加载读取的值
        // %result = load i32, i32* %temp
        std::string result = emitter_.emit_load("i32", temp_var);

        store_expr_result(node, result);
        return true;
    }

    // exit(code: i32) -> ()
    // 终止程序执行
    if (func_name == "exit") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        // 调用 exit
        // call void @exit(i32 %code)
        emitter_.emit_call_void("exit", {{"i32", arg_value}});

        // exit 后代码不可达，添加 unreachable 指令作为块终止符
        // unreachable
        emitter_.emit_unreachable();

        // 标记当前块已终止（不再添加其他指令）
        current_block_terminated_ = true;

        store_expr_result(node, "");
        return true;
    }

    return false;
}
