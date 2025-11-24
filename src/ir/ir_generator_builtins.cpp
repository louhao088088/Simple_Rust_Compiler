#include "ir_generator.h"

// Emit declarations for C library functions used by built-in functions.
void IRGenerator::emit_builtin_declarations() {
    emitter_.emit_function_declaration("i32", "printf", {"i8*"}, true);
    emitter_.emit_function_declaration("i32", "scanf", {"i8*"}, true);
    emitter_.emit_function_declaration("void", "exit", {"i32"}, false);
    emitter_.emit_function_declaration("void", "llvm.memset.p0.i64", {"i8*", "i8", "i64", "i1"},
                                       false);
    emitter_.emit_function_declaration("void", "llvm.memcpy.p0.p0.i64", {"i8*", "i8*", "i64", "i1"},
                                       false);

    emitter_.emit_blank_line();

    emitter_.emit_global_variable(".str.int", "[3 x i8]", "c\"%d\\00\"", true);
    emitter_.emit_global_variable(".str.int_newline", "[4 x i8]", "c\"%d\\0A\\00\"", true);
    emitter_.emit_global_variable(".str.int_scanf", "[3 x i8]", "c\"%d\\00\"", true);

    emitter_.emit_blank_line();
}

bool IRGenerator::handle_builtin_function(
    CallExpr *node, const std::string &func_name,
    const std::vector<std::pair<std::string, std::string>> &args) {
    if (func_name == "printInt") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        std::string fmt_ptr =
            emitter_.emit_getelementptr("[3 x i8]", "@.str.int", {"i32 0", "i32 0"});

        emitter_.emit_vararg_call("i32", "printf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32", arg_value}});

        store_expr_result(node, "");
        return true;
    }

    if (func_name == "printlnInt") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        std::string fmt_ptr =
            emitter_.emit_getelementptr("[4 x i8]", "@.str.int_newline", {"i32 0", "i32 0"});

        emitter_.emit_vararg_call("i32", "printf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32", arg_value}});

        store_expr_result(node, "");
        return true;
    }

    if (func_name == "getInt") {
        if (args.size() != 0) {
            return false;
        }

        std::string temp_var = emitter_.emit_alloca("i32");

        std::string fmt_ptr =
            emitter_.emit_getelementptr("[3 x i8]", "@.str.int_scanf", {"i32 0", "i32 0"});

        emitter_.emit_vararg_call("i32", "scanf", "(i8*, ...)",
                                  {{"i8*", fmt_ptr}, {"i32*", temp_var}});

        std::string result = emitter_.emit_load("i32", temp_var);

        store_expr_result(node, result);
        return true;
    }

    if (func_name == "exit") {
        if (args.size() != 1) {
            return false;
        }

        std::string arg_value = args[0].second;

        if (arg_value.empty()) {
            return false;
        }

        emitter_.emit_call_void("exit", {{"i32", arg_value}});

        emitter_.emit_unreachable();

        current_block_terminated_ = true;

        store_expr_result(node, "");
        return true;
    }

    return false;
}
