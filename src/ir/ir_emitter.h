#pragma once
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * IREmitter - LLVM IR文本生成器
 *
 * 核心职责:
 * 1. 直接输出LLVM IR文本格式(不使用LLVM C++ API)
 * 2. 管理临时变量命名(%0, %1, %2...)
 * 3. 管理基本块标签命名(label0, label1...)
 * 4. 提供各种IR指令的文本生成方法
 * 5. 维护缩进使输出可读
 */
class IREmitter {
  public:
    /**
     * 构造函数
     * @param module_name 模块名称
     */
    explicit IREmitter(const std::string &module_name);

    // ========== Module级别 ==========

    /**
     * 输出全局变量声明
     * 例: @global_var = global i32 0
     */
    void emit_global_variable(const std::string &name, const std::string &type,
                              const std::string &initializer, bool is_constant = false);

    /**
     * 输出结构体类型定义
     * 例: %Point = type { i32, i32 }
     */
    void emit_struct_type(const std::string &name, const std::vector<std::string> &field_types);

    /**
     * 输出函数声明(不含函数体)
     * 例: declare i32 @printf(i8*, ...)
     */
    void emit_function_declaration(const std::string &return_type, const std::string &name,
                                   const std::vector<std::string> &param_types,
                                   bool is_vararg = false);

    // ========== 函数级别 ==========

    /**
     * 开始函数定义
     * 例: define i32 @main(i32 %argc, i8** %argv) {
     */
    void begin_function(const std::string &return_type, const std::string &name,
                        const std::vector<std::pair<std::string, std::string>> &params);

    /**
     * 完成entry块，输出缓冲的alloca指令
     * 应在entry块的所有alloca后、其他指令前调用
     */
    void finish_entry_block();

    /**
     * 结束函数定义
     * 输出: }
     */
    void end_function();

    // ========== 基本块 ==========

    /**
     * 创建并进入新的基本块
     * 例: entry:
     */
    void begin_basic_block(const std::string &label);

    // ========== 内存操作指令 ==========

    /**
     * alloca指令: 在栈上分配内存
     * @return 分配的指针变量名(如%0)
     * 例: %0 = alloca i32
     */
    std::string emit_alloca(const std::string &type, const std::string &var_name = "");

    /**
     * store指令: 存储值到内存
     * 例: store i32 42, i32* %0
     */
    void emit_store(const std::string &value_type, const std::string &value,
                    const std::string &ptr);

    /**
     * load指令: 从内存加载值
     * @return 加载的值变量名(如%1)
     * 例: %1 = load i32, i32* %0
     */
    std::string emit_load(const std::string &type, const std::string &ptr);

    // ========== 算术和逻辑运算 ==========

    /**
     * 二元运算指令
     * @param op 操作符(add, sub, mul, sdiv, srem, udiv, urem, and, or, xor等)
     * @return 结果变量名
     * 例: %2 = add i32 %0, %1
     */
    std::string emit_binary_op(const std::string &op, const std::string &type,
                               const std::string &lhs, const std::string &rhs);

    /**
     * 整数比较指令
     * @param predicate 比较谓词(eq, ne, slt, sle, sgt, sge, ult, ule, ugt, uge)
     * @return 比较结果(i1类型)
     * 例: %3 = icmp eq i32 %0, %1
     */
    std::string emit_icmp(const std::string &predicate, const std::string &type,
                          const std::string &lhs, const std::string &rhs);

    /**
     * 一元取负运算
     * @return 结果变量名
     * 例: %4 = sub i32 0, %0
     */
    std::string emit_neg(const std::string &type, const std::string &operand);

    /**
     * 逻辑非运算(针对i1)
     * @return 结果变量名
     * 例: %5 = xor i1 %0, true
     */
    std::string emit_not(const std::string &operand);

    // ========== 类型转换 ==========

    /**
     * 截断转换(窄化)
     * 例: %6 = trunc i64 %0 to i32
     */
    std::string emit_trunc(const std::string &from_type, const std::string &value,
                           const std::string &to_type);

    /**
     * 零扩展(无符号扩展)
     * 例: %7 = zext i32 %0 to i64
     */
    std::string emit_zext(const std::string &from_type, const std::string &value,
                          const std::string &to_type);

    /**
     * 符号扩展(有符号扩展)
     * 例: %8 = sext i32 %0 to i64
     */
    std::string emit_sext(const std::string &from_type, const std::string &value,
                          const std::string &to_type);

    /**
     * 位转换(指针类型转换)
     * 例: %9 = bitcast [100 x i32]* %0 to i8*
     */
    std::string emit_bitcast(const std::string &from_type, const std::string &value,
                             const std::string &to_type);

    // ========== 控制流 ==========

    /**
     * 返回指令(带返回值)
     * 例: ret i32 0
     */
    void emit_ret(const std::string &type, const std::string &value);

    /**
     * 返回void
     * 例: ret void
     */
    void emit_ret_void();

    /**
     * 无条件跳转
     * 例: br label %label1
     */
    void emit_br(const std::string &target_label);

    /**
     * 条件跳转
     * 例: br i1 %cond, label %true_bb, label %false_bb
     */
    void emit_cond_br(const std::string &condition, const std::string &true_label,
                      const std::string &false_label);

    /**
     * PHI节点(用于控制流合并)
     * @param incoming 格式: [(value1, label1), (value2, label2), ...]
     * @return PHI节点结果变量名
     * 例: %9 = phi i32 [%0, %label1], [%1, %label2]
     */
    std::string emit_phi(const std::string &type,
                         const std::vector<std::pair<std::string, std::string>> &incoming);

    /**
     * unreachable指令(标记不可达代码)
     * 例: unreachable
     */
    void emit_unreachable();

    // ========== 函数调用 ==========

    /**
     * call指令(有返回值)
     * @param args 格式: [(type1, value1), (type2, value2), ...]
     * @return 调用结果变量名
     * 例: %10 = call i32 @add(i32 %0, i32 %1)
     */
    std::string emit_call(const std::string &return_type, const std::string &func_name,
                          const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * call指令(无返回值/void)
     * 例: call void @print(i32 %0)
     */
    void emit_call_void(const std::string &func_name,
                        const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * 生成vararg函数调用(用于printf, scanf等)
     * 例: %0 = call i32 (i8*, ...) @printf(i8* %fmt, i32 42)
     */
    std::string emit_vararg_call(const std::string &return_type, const std::string &func_name,
                                 const std::string &func_type,
                                 const std::vector<std::pair<std::string, std::string>> &args);

    // ========== 指针和数组操作 ==========

    /**
     * getelementptr指令(计算元素地址)
     * @param indices 索引列表(格式: "i64 0", "i32 1"等)
     * @return 计算出的指针
     * 例: %11 = getelementptr [10 x i32], [10 x i32]* %arr, i64 0, i64 5
     */
    std::string emit_getelementptr(const std::string &type, const std::string &ptr,
                                   const std::vector<std::string> &indices);

    /**
     * getelementptr inbounds指令(带边界检查优化提示)
     */
    std::string emit_getelementptr_inbounds(const std::string &type, const std::string &ptr,
                                            const std::vector<std::string> &indices);

    // ========== 临时变量和标签管理 ==========

    /**
     * 获取新的临时变量名
     * @return 格式: %0, %1, %2, ...
     */
    std::string new_temp();

    /**
     * 获取新的标签名
     * @return 格式: label0, label1, label2, ...
     */
    std::string new_label();

    /**
     * 重置临时变量计数器(每个函数开始时调用)
     */
    void reset_temp_counter();

    // ========== 注释 ==========

    /**
     * 添加注释行
     * 例: ; This is a comment
     */
    void emit_comment(const std::string &comment);

    /**
     * 添加空行(用于提高可读性)
     */
    void emit_blank_line();

    // ========== 输出 ==========

    /**
     * 将累积的IR输出到文件
     */
    void write_to_file(const std::string &filename);

    /**
     * 输出到stdout(用于调试)
     */
    void write_to_stdout();

    /**
     * 获取生成的IR字符串
     */
    std::string get_ir_string() const;

  private:
    std::string module_name_;
    std::stringstream ir_stream_; // 累积的IR文本

    int temp_counter_;  // 临时变量计数器
    int label_counter_; // 标签计数器
    int indent_level_;  // 当前缩进层级

    // alloca缓冲机制（用于提升alloca到entry块）
    bool in_entry_block_;                    // 是否在entry块中
    std::vector<std::string> alloca_buffer_; // 缓冲的alloca指令
    std::stringstream instruction_buffer_;   // 非alloca指令缓冲

    // 辅助方法

    /**
     * 输出一行(带缩进)
     */
    void emit_line(const std::string &line);

    /**
     * 直接输出(不带缩进,用于标签等)
     */
    void emit_raw(const std::string &text);

    /**
     * 获取当前缩进字符串
     */
    std::string indent() const;
};
