#include "ir_emitter.h"

#include <fstream>
#include <iostream>

IREmitter::IREmitter(const std::string &module_name)
    : module_name_(module_name), temp_counter_(0), label_counter_(0), indent_level_(0) {
    // 输出Module头部
    ir_stream_ << "; ModuleID = '" << module_name_ << "'\n";
    ir_stream_ << "source_filename = \"" << module_name_ << "\"\n\n";
}

// ========== Module级别 ==========

void IREmitter::emit_global_variable(const std::string &name, const std::string &type,
                                     const std::string &initializer, bool is_constant) {
    ir_stream_ << "@" << name << " = ";
    ir_stream_ << (is_constant ? "constant " : "global ");
    ir_stream_ << type << " " << initializer << "\n";
}

void IREmitter::emit_struct_type(const std::string &name,
                                 const std::vector<std::string> &field_types) {
    ir_stream_ << "%" << name << " = type { ";
    for (size_t i = 0; i < field_types.size(); ++i) {
        ir_stream_ << field_types[i];
        if (i + 1 < field_types.size()) {
            ir_stream_ << ", ";
        }
    }
    ir_stream_ << " }\n";
}

void IREmitter::emit_function_declaration(const std::string &return_type, const std::string &name,
                                          const std::vector<std::string> &param_types,
                                          bool is_vararg) {
    ir_stream_ << "declare " << return_type << " @" << name << "(";
    for (size_t i = 0; i < param_types.size(); ++i) {
        ir_stream_ << param_types[i];
        if (i + 1 < param_types.size()) {
            ir_stream_ << ", ";
        }
    }
    if (is_vararg) {
        if (!param_types.empty()) {
            ir_stream_ << ", ";
        }
        ir_stream_ << "...";
    }
    ir_stream_ << ")\n";
}

// ========== 函数级别 ==========

void IREmitter::begin_function(const std::string &return_type, const std::string &name,
                               const std::vector<std::pair<std::string, std::string>> &params) {
    // define return_type @name(type1 %param1, type2 %param2, ...) {
    ir_stream_ << "\ndefine " << return_type << " @" << name << "(";

    for (size_t i = 0; i < params.size(); ++i) {
        ir_stream_ << params[i].first << " %" << params[i].second;
        if (i + 1 < params.size()) {
            ir_stream_ << ", ";
        }
    }

    ir_stream_ << ") {\n";
    indent_level_++;

    // 重置临时变量计数器(每个函数独立编号)
    reset_temp_counter();
}

void IREmitter::end_function() {
    indent_level_--;
    ir_stream_ << "}\n";
}

// ========== 基本块 ==========

void IREmitter::begin_basic_block(const std::string &label) {
    // 基本块标签不缩进,直接输出
    // 但如果不是函数的第一个基本块,要减少缩进
    if (indent_level_ > 0) {
        indent_level_--;
    }
    ir_stream_ << label << ":\n";
    indent_level_++;
}

// ========== 内存操作指令 ==========

std::string IREmitter::emit_alloca(const std::string &type, const std::string &var_name) {
    std::string result = new_temp();
    std::string line = result + " = alloca " + type;
    if (!var_name.empty()) {
        line += " ; " + var_name;
    }
    emit_line(line);
    return result;
}

void IREmitter::emit_store(const std::string &value_type, const std::string &value,
                           const std::string &ptr) {
    emit_line("store " + value_type + " " + value + ", " + value_type + "* " + ptr);
}

std::string IREmitter::emit_load(const std::string &type, const std::string &ptr) {
    std::string result = new_temp();
    emit_line(result + " = load " + type + ", " + type + "* " + ptr);
    return result;
}

// ========== 算术和逻辑运算 ==========

std::string IREmitter::emit_binary_op(const std::string &op, const std::string &type,
                                      const std::string &lhs, const std::string &rhs) {
    std::string result = new_temp();
    emit_line(result + " = " + op + " " + type + " " + lhs + ", " + rhs);
    return result;
}

std::string IREmitter::emit_icmp(const std::string &predicate, const std::string &type,
                                 const std::string &lhs, const std::string &rhs) {
    std::string result = new_temp();
    emit_line(result + " = icmp " + predicate + " " + type + " " + lhs + ", " + rhs);
    return result;
}

std::string IREmitter::emit_neg(const std::string &type, const std::string &operand) {
    // 取负: sub type 0, operand
    return emit_binary_op("sub", type, "0", operand);
}

std::string IREmitter::emit_not(const std::string &operand) {
    // 逻辑非: xor i1 operand, true
    return emit_binary_op("xor", "i1", operand, "true");
}

// ========== 类型转换 ==========

std::string IREmitter::emit_trunc(const std::string &from_type, const std::string &value,
                                  const std::string &to_type) {
    std::string result = new_temp();
    emit_line(result + " = trunc " + from_type + " " + value + " to " + to_type);
    return result;
}

std::string IREmitter::emit_zext(const std::string &from_type, const std::string &value,
                                 const std::string &to_type) {
    std::string result = new_temp();
    emit_line(result + " = zext " + from_type + " " + value + " to " + to_type);
    return result;
}

std::string IREmitter::emit_sext(const std::string &from_type, const std::string &value,
                                 const std::string &to_type) {
    std::string result = new_temp();
    emit_line(result + " = sext " + from_type + " " + value + " to " + to_type);
    return result;
}

// ========== 控制流 ==========

void IREmitter::emit_ret(const std::string &type, const std::string &value) {
    emit_line("ret " + type + " " + value);
}

void IREmitter::emit_ret_void() { emit_line("ret void"); }

void IREmitter::emit_br(const std::string &target_label) { emit_line("br label %" + target_label); }

void IREmitter::emit_cond_br(const std::string &condition, const std::string &true_label,
                             const std::string &false_label) {
    emit_line("br i1 " + condition + ", label %" + true_label + ", label %" + false_label);
}

std::string IREmitter::emit_phi(const std::string &type,
                                const std::vector<std::pair<std::string, std::string>> &incoming) {
    std::string result = new_temp();
    std::string phi_str = result + " = phi " + type + " ";

    for (size_t i = 0; i < incoming.size(); ++i) {
        phi_str += "[" + incoming[i].first + ", %" + incoming[i].second + "]";
        if (i + 1 < incoming.size()) {
            phi_str += ", ";
        }
    }

    emit_line(phi_str);
    return result;
}

void IREmitter::emit_unreachable() { emit_line("unreachable"); }

// ========== 函数调用 ==========

std::string IREmitter::emit_call(const std::string &return_type, const std::string &func_name,
                                 const std::vector<std::pair<std::string, std::string>> &args) {
    std::string result = new_temp();
    std::string call_str = result + " = call " + return_type + " @" + func_name + "(";

    for (size_t i = 0; i < args.size(); ++i) {
        call_str += args[i].first + " " + args[i].second;
        if (i + 1 < args.size()) {
            call_str += ", ";
        }
    }

    call_str += ")";
    emit_line(call_str);
    return result;
}

void IREmitter::emit_call_void(const std::string &func_name,
                               const std::vector<std::pair<std::string, std::string>> &args) {
    std::string call_str = "call void @" + func_name + "(";

    for (size_t i = 0; i < args.size(); ++i) {
        call_str += args[i].first + " " + args[i].second;
        if (i + 1 < args.size()) {
            call_str += ", ";
        }
    }

    call_str += ")";
    emit_line(call_str);
}

// ========== 指针和数组操作 ==========

std::string IREmitter::emit_getelementptr(const std::string &type, const std::string &ptr,
                                          const std::vector<std::string> &indices) {
    std::string result = new_temp();
    std::string gep_str = result + " = getelementptr " + type + ", " + type + "* " + ptr;

    for (const auto &idx : indices) {
        gep_str += ", " + idx;
    }

    emit_line(gep_str);
    return result;
}

std::string IREmitter::emit_getelementptr_inbounds(const std::string &type, const std::string &ptr,
                                                   const std::vector<std::string> &indices) {
    std::string result = new_temp();
    std::string gep_str = result + " = getelementptr inbounds " + type + ", " + type + "* " + ptr;

    for (const auto &idx : indices) {
        gep_str += ", " + idx;
    }

    emit_line(gep_str);
    return result;
}

// ========== 临时变量和标签管理 ==========

std::string IREmitter::new_temp() { return "%" + std::to_string(temp_counter_++); }

std::string IREmitter::new_label() { return "label" + std::to_string(label_counter_++); }

void IREmitter::reset_temp_counter() { temp_counter_ = 0; }

// ========== 注释 ==========

void IREmitter::emit_comment(const std::string &comment) { emit_line("; " + comment); }

void IREmitter::emit_blank_line() { ir_stream_ << "\n"; }

// ========== 输出 ==========

void IREmitter::write_to_file(const std::string &filename) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return;
    }
    out << ir_stream_.str();
    out.close();
}

void IREmitter::write_to_stdout() { std::cout << ir_stream_.str(); }

std::string IREmitter::get_ir_string() const { return ir_stream_.str(); }

// ========== 私有辅助方法 ==========

void IREmitter::emit_line(const std::string &line) { ir_stream_ << indent() << line << "\n"; }

void IREmitter::emit_raw(const std::string &text) { ir_stream_ << text; }

std::string IREmitter::indent() const {
    return std::string(indent_level_ * 2, ' '); // 每层缩进2个空格
}
