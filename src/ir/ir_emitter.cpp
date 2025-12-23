#include "ir_emitter.h"

#include <fstream>
#include <iostream>

IREmitter::IREmitter(const std::string &module_name)
    : module_name_(module_name), temp_counter_(0), label_counter_(0), stack_counter_(0),
      trampoline_counter_(0), indent_level_(0), in_entry_block_(false), is_inside_function_(false) {
    ir_stream_ << "; ModuleID = '" << module_name_ << "'\n";
    ir_stream_ << "source_filename = \"" << module_name_ << "\"\n\n";
}

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

void IREmitter::begin_function(const std::string &return_type, const std::string &name,
                               const std::vector<std::pair<std::string, std::string>> &params) {
    is_inside_function_ = true;
    function_body_buffer_.str("");
    function_body_buffer_.clear();
    function_allocas_.clear();

    ir_stream_ << "\ndefine " << return_type << " @" << name << "(";

    for (size_t i = 0; i < params.size(); ++i) {
        ir_stream_ << params[i].first << " %" << params[i].second;
        if (i + 1 < params.size()) {
            ir_stream_ << ", ";
        }
    }

    ir_stream_ << ") {\n";
    indent_level_++;

    reset_temp_counter();

    alloca_buffer_.clear();
    instruction_buffer_.str("");
    instruction_buffer_.clear();
    in_entry_block_ = false;
}

void IREmitter::finish_entry_block() {}

void IREmitter::end_function() {
    indent_level_--;

    std::string body = function_body_buffer_.str();
    size_t pos = body.find(":\n");

    if (pos != std::string::npos) {
        size_t insert_pos = pos + 2;
        ir_stream_ << body.substr(0, insert_pos);

        for (const auto &alloca_instr : function_allocas_) {
            ir_stream_ << "  " << alloca_instr;
        }

        ir_stream_ << body.substr(insert_pos);
    } else {
        for (const auto &alloca_instr : function_allocas_) {
            ir_stream_ << "  " << alloca_instr;
        }
        ir_stream_ << body;
    }

    ir_stream_ << "}\n";
    is_inside_function_ = false;
}

void IREmitter::begin_basic_block(const std::string &label) {
    if (indent_level_ > 0) {
        indent_level_--;
    }

    if (is_inside_function_) {
        function_body_buffer_ << label << ":\n";
    } else {
        ir_stream_ << label << ":\n";
    }

    indent_level_++;
}

std::string IREmitter::emit_alloca(const std::string &type, const std::string &var_name) {
    std::string result = "%stack." + std::to_string(stack_counter_++);

    std::string line = result + " = alloca " + type;
    if (!var_name.empty()) {
        line += " ; " + var_name;
    }
    line += "\n";

    if (is_inside_function_) {
        function_allocas_.push_back(line);
    } else {
        emit_line(line);
    }

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

void IREmitter::emit_memcpy(const std::string &dest_ptr, const std::string &src_ptr, size_t bytes,
                            const std::string &ptr_type) {
    std::string dest_i8 = emit_bitcast(ptr_type, dest_ptr, "i8*");
    std::string src_i8 = emit_bitcast(ptr_type, src_ptr, "i8*");

    std::stringstream ss;
    ss << "call void @llvm.memcpy.p0.p0.i64(i8* " << dest_i8 << ", i8* " << src_i8 << ", i64 "
       << bytes << ", i1 false)";
    emit_line(ss.str());
}

void IREmitter::emit_memset(const std::string &dest_ptr, int value, size_t bytes,
                            const std::string &ptr_type) {
    std::string dest_i8 = emit_bitcast(ptr_type, dest_ptr, "i8*");

    std::stringstream ss;
    ss << "call void @llvm.memset.p0.i64(i8* " << dest_i8 << ", i8 " << value << ", i64 " << bytes
       << ", i1 false)";
    emit_line(ss.str());
}

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
    return emit_binary_op("sub", type, "0", operand);
}

std::string IREmitter::emit_not(const std::string &operand) {
    return emit_binary_op("xor", "i1", operand, "true");
}

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

std::string IREmitter::emit_bitcast(const std::string &from_type, const std::string &value,
                                    const std::string &to_type) {
    std::string result = new_temp();
    emit_line(result + " = bitcast " + from_type + " " + value + " to " + to_type);
    return result;
}

void IREmitter::emit_ret(const std::string &type, const std::string &value) {
    emit_line("ret " + type + " " + value);
}

void IREmitter::emit_ret_void() { emit_line("ret void"); }

void IREmitter::emit_br(const std::string &target_label) { emit_line("br label %" + target_label); }

std::pair<std::string, std::string> IREmitter::emit_cond_br(const std::string &condition, 
                                                              const std::string &true_label,
                                                              const std::string &false_label) {
    // Use trampoline blocks to avoid RISC-V beq/bne ±4KB range limitation
    // beq/bne can only jump ±4KB, but jal can jump ±1MB
    // Pattern:
    //   br i1 %cond, label %jmp_true_N, label %jmp_false_N
    //   jmp_true_N:  br label %true_label   (will become: jal true_label)
    //   jmp_false_N: br label %false_label  (will become: jal false_label)
    
    size_t tramp_id = trampoline_counter_++;
    std::string jmp_true = "jmp_true_" + std::to_string(tramp_id);
    std::string jmp_false = "jmp_false_" + std::to_string(tramp_id);
    
    // Conditional branch to nearby trampoline blocks
    emit_line("br i1 " + condition + ", label %" + jmp_true + ", label %" + jmp_false);
    
    // Trampoline block for true branch
    if (indent_level_ > 0) indent_level_--;
    if (is_inside_function_) {
        function_body_buffer_ << jmp_true << ":\n";
    } else {
        ir_stream_ << jmp_true << ":\n";
    }
    indent_level_++;
    emit_line("br label %" + true_label);
    
    // Trampoline block for false branch
    if (indent_level_ > 0) indent_level_--;
    if (is_inside_function_) {
        function_body_buffer_ << jmp_false << ":\n";
    } else {
        ir_stream_ << jmp_false << ":\n";
    }
    indent_level_++;
    emit_line("br label %" + false_label);
    
    // Return the trampoline labels for PHI node predecessors
    return {jmp_true, jmp_false};
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

std::string
IREmitter::emit_vararg_call(const std::string &return_type, const std::string &func_name,
                            const std::string &func_type,
                            const std::vector<std::pair<std::string, std::string>> &args) {
    std::string result = new_temp();
    std::string call_str =
        result + " = call " + return_type + " " + func_type + " @" + func_name + "(";

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

std::string IREmitter::new_temp() { return "%" + std::to_string(temp_counter_++); }

std::string IREmitter::new_label() { return "label" + std::to_string(label_counter_++); }

void IREmitter::reset_temp_counter() {
    temp_counter_ = 0;
    stack_counter_ = 0;
}

void IREmitter::emit_comment(const std::string &comment) { emit_line("; " + comment); }

void IREmitter::emit_blank_line() { ir_stream_ << "\n"; }

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

void IREmitter::emit_line(const std::string &line) {
    if (is_inside_function_) {
        function_body_buffer_ << indent() << line << "\n";
    } else {
        ir_stream_ << indent() << line << "\n";
    }
}

void IREmitter::emit_raw(const std::string &text) {
    if (is_inside_function_) {
        function_body_buffer_ << text;
    } else {
        ir_stream_ << text;
    }
}

std::string IREmitter::indent() const { return std::string(indent_level_ * 2, ' '); }
