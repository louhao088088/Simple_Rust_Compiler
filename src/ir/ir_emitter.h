#pragma once
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * IREmitter - LLVM IR Text Generator
 *
 * Core responsibilities:
 * 1. Output LLVM IR text format directly (without using LLVM C++ API)
 * 2. Manage temporary variable naming (%0, %1, %2...)
 * 3. Manage basic block label naming (label0, label1...)
 * 4. Provide text generation methods for various IR instructions
 * 5. Maintain indentation for readable output
 */
class IREmitter {
  public:
    /**
     * Constructor
     * @param module_name Module name
     */
    explicit IREmitter(const std::string &module_name);

    /**
     * Emit global variable declaration
     * Example: @global_var = global i32 0
     */
    void emit_global_variable(const std::string &name, const std::string &type,
                              const std::string &initializer, bool is_constant = false);

    /**
     * Emit struct type definition
     * Example: %Point = type { i32, i32 }
     */
    void emit_struct_type(const std::string &name, const std::vector<std::string> &field_types);

    /**
     * Emit function declaration (without function body)
     * Example: declare i32 @printf(i8*, ...)
     */
    void emit_function_declaration(const std::string &return_type, const std::string &name,
                                   const std::vector<std::string> &param_types,
                                   bool is_vararg = false);

    /**
     * Begin function definition
     * Example: define i32 @main(i32 %argc, i8** %argv) {
     */
    void begin_function(const std::string &return_type, const std::string &name,
                        const std::vector<std::pair<std::string, std::string>> &params);

    /**
     * Finish entry block, output buffered alloca instructions
     * Should be called after all allocas in entry block, before other instructions
     */
    void finish_entry_block();

    /**
     * End function definition
     * Output: }
     */
    void end_function();

    /**
     * Create and enter a new basic block
     * Example: entry:
     */
    void begin_basic_block(const std::string &label);

    /**
     * alloca instruction: Allocate memory on stack
     * @return Allocated pointer variable name (e.g., %0)
     * Example: %0 = alloca i32
     */
    std::string emit_alloca(const std::string &type, const std::string &var_name = "");

    /**
     * store instruction: Store value to memory
     * Example: store i32 42, i32* %0
     */
    void emit_store(const std::string &value_type, const std::string &value,
                    const std::string &ptr);

    /**
     * load instruction: Load value from memory
     * @return Loaded value variable name (e.g., %1)
     * Example: %1 = load i32, i32* %0
     */
    std::string emit_load(const std::string &type, const std::string &ptr);

    /**
     * memcpy instruction: Memory copy
     * Uses llvm.memcpy.p0.p0.i64
     */
    void emit_memcpy(const std::string &dest_ptr, const std::string &src_ptr, size_t bytes,
                     const std::string &ptr_type);

    /**
     * memset instruction: Memory set
     * Uses llvm.memset.p0.i64
     */
    void emit_memset(const std::string &dest_ptr, int value, size_t bytes,
                     const std::string &ptr_type);

    /**
     * Binary operation instruction
     * @param op Operator (add, sub, mul, sdiv, srem, udiv, urem, and, or, xor, etc.)
     * @return Result variable name
     * Example: %2 = add i32 %0, %1
     */
    std::string emit_binary_op(const std::string &op, const std::string &type,
                               const std::string &lhs, const std::string &rhs);

    /**
     * Integer comparison instruction
     * @param predicate Comparison predicate (eq, ne, slt, sle, sgt, sge, ult, ule, ugt, uge)
     * @return Comparison result (i1 type)
     * Example: %3 = icmp eq i32 %0, %1
     */
    std::string emit_icmp(const std::string &predicate, const std::string &type,
                          const std::string &lhs, const std::string &rhs);

    /**
     * Unary negation operation
     * @return Result variable name
     * Example: %4 = sub i32 0, %0
     */
    std::string emit_neg(const std::string &type, const std::string &operand);

    /**
     * Logical NOT operation (for i1)
     * @return Result variable name
     * Example: %5 = xor i1 %0, true
     */
    std::string emit_not(const std::string &operand);

    /**
     * Truncation conversion (narrowing)
     * Example: %6 = trunc i64 %0 to i32
     */
    std::string emit_trunc(const std::string &from_type, const std::string &value,
                           const std::string &to_type);

    /**
     * Zero extension (unsigned extension)
     * Example: %7 = zext i32 %0 to i64
     */
    std::string emit_zext(const std::string &from_type, const std::string &value,
                          const std::string &to_type);

    /**
     * Sign extension (signed extension)
     * Example: %8 = sext i32 %0 to i64
     */
    std::string emit_sext(const std::string &from_type, const std::string &value,
                          const std::string &to_type);

    /**
     * Bitcast conversion (pointer type conversion)
     * Example: %9 = bitcast [100 x i32]* %0 to i8*
     */
    std::string emit_bitcast(const std::string &from_type, const std::string &value,
                             const std::string &to_type);

    /**
     * Return instruction (with return value)
     * Example: ret i32 0
     */
    void emit_ret(const std::string &type, const std::string &value);

    /**
     * Return void
     * Example: ret void
     */
    void emit_ret_void();

    /**
     * Unconditional branch
     * Example: br label %label1
     */
    void emit_br(const std::string &target_label);

    /**
     * Conditional branch with trampoline blocks for long jumps
     * Uses trampoline blocks to avoid RISC-V beq/bne ±4KB range limitation
     * @return pair of (true_trampoline_label, false_trampoline_label) for PHI predecessors
     * Example: br i1 %cond, label %jmp_true_N, label %jmp_false_N
     */
    std::pair<std::string, std::string> emit_cond_br(const std::string &condition, 
                                                      const std::string &true_label,
                                                      const std::string &false_label);

    /**
     * PHI node (for control flow merging)
     * @param incoming Format: [(value1, label1), (value2, label2), ...]
     * @return PHI node result variable name
     * Example: %9 = phi i32 [%0, %label1], [%1, %label2]
     */
    std::string emit_phi(const std::string &type,
                         const std::vector<std::pair<std::string, std::string>> &incoming);

    /**
     * unreachable instruction (marks unreachable code)
     * Example: unreachable
     */
    void emit_unreachable();

    /**
     * call instruction (with return value)
     * @param args Format: [(type1, value1), (type2, value2), ...]
     * @return Call result variable name
     * Example: %10 = call i32 @add(i32 %0, i32 %1)
     */
    std::string emit_call(const std::string &return_type, const std::string &func_name,
                          const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * call instruction (no return value/void)
     * Example: call void @print(i32 %0)
     */
    void emit_call_void(const std::string &func_name,
                        const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * Generate vararg function call (for printf, scanf, etc.)
     * Example: %0 = call i32 (i8*, ...) @printf(i8* %fmt, i32 42)
     */
    std::string emit_vararg_call(const std::string &return_type, const std::string &func_name,
                                 const std::string &func_type,
                                 const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * getelementptr instruction (calculate element address)
     * @param indices Index list (format: "i64 0", "i32 1", etc.)
     * @return Calculated pointer
     * Example: %11 = getelementptr [10 x i32], [10 x i32]* %arr, i64 0, i64 5
     */
    std::string emit_getelementptr(const std::string &type, const std::string &ptr,
                                   const std::vector<std::string> &indices);

    /**
     * getelementptr inbounds instruction (with bounds check optimization hint)
     */
    std::string emit_getelementptr_inbounds(const std::string &type, const std::string &ptr,
                                            const std::vector<std::string> &indices);

    /**
     * Get a new temporary variable name
     * @return Format: %0, %1, %2, ...
     */
    std::string new_temp();

    /**
     * Get a new label name
     * @return Format: label0, label1, label2, ...
     */
    std::string new_label();

    /**
     * Reset temporary variable counter (called at the start of each function)
     */
    void reset_temp_counter();

    /**
     * Add a comment line
     * Example: ; This is a comment
     */
    void emit_comment(const std::string &comment);

    /**
     * Add a blank line (for readability)
     */
    void emit_blank_line();

    /**
     * Write accumulated IR to file
     */
    void write_to_file(const std::string &filename);

    /**
     * Output to stdout (for debugging)
     */
    void write_to_stdout();

    /**
     * Get the generated IR string
     */
    std::string get_ir_string() const;

  private:
    std::string module_name_;
    std::stringstream ir_stream_;

    size_t temp_counter_;
    size_t label_counter_;
    size_t stack_counter_;
    size_t trampoline_counter_;  // Counter for branch trampoline labels
    int indent_level_;

    bool in_entry_block_;
    std::vector<std::string> alloca_buffer_;
    std::stringstream instruction_buffer_;

    bool is_inside_function_;
    std::stringstream function_body_buffer_;
    std::vector<std::string> function_allocas_;

    /**
     * Output a line (with indentation)
     */
    void emit_line(const std::string &line);

    /**
     * Direct output (without indentation, used for labels, etc.)
     */
    void emit_raw(const std::string &text);

    /**
     * Get current indentation string
     */
    std::string indent() const;
};
