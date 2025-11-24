#pragma once

#include "../ast/ast.h"
#include "../ast/visit.h"
#include "../semantic/semantic.h"
#include "ir_emitter.h"
#include "type_mapper.h"
#include "value_manager.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * IRGenerator - LLVM IR 生成器
 *
 * 核心职责:
 * 1. 遍历 AST 生成 LLVM IR 文本
 * 2. 协调 IREmitter、TypeMapper、ValueManager
 * 3. 处理表达式、语句、函数定义
 *
 * 设计原则：
 * - 使用访问者模式遍历 AST
 * - 表达式结果存储在 expr_results_ 中，不通过返回值传递
 * - 直接使用 AST 节点中的 type 字段获取类型信息
 * - 不使用 LLVM C++ API，生成纯文本 IR
 */
class IRGenerator : public ExprVisitor<void>, public StmtVisitor {
  public:
    /**
     * 构造函数
     * @param builtin_types 内置类型引用（来自语义分析器）
     */
    explicit IRGenerator(BuiltinTypes &builtin_types);

    /**
     * 生成完整程序的 IR
     * @param program AST 根节点
     * @return 生成的 LLVM IR 文本
     */
    std::string generate(Program *program);


    void visit(LiteralExpr *node) override;
    void visit(ArrayLiteralExpr *node) override;
    void visit(ArrayInitializerExpr *node) override;
    void visit(VariableExpr *node) override;
    void visit(UnaryExpr *node) override;
    void visit(BinaryExpr *node) override;
    void visit(CallExpr *node) override;
    void visit(IfExpr *node) override;
    void visit(LoopExpr *node) override;
    void visit(WhileExpr *node) override;
    void visit(IndexExpr *node) override;
    void visit(FieldAccessExpr *node) override;
    void visit(AssignmentExpr *node) override;
    void visit(CompoundAssignmentExpr *node) override;
    void visit(ReferenceExpr *node) override;
    void visit(UnderscoreExpr *node) override;
    void visit(AsExpr *node) override;
    void visit(StructInitializerExpr *node) override;
    void visit(UnitExpr *node) override;
    void visit(GroupingExpr *node) override;
    void visit(TupleExpr *node) override;
    void visit(MatchExpr *node) override;
    void visit(PathExpr *node) override;
    void visit(BlockExpr *node) override;


    void visit(BlockStmt *node) override;
    void visit(ExprStmt *node) override;
    void visit(LetStmt *node) override;
    void visit(ReturnStmt *node) override;
    void visit(BreakStmt *node) override;
    void visit(ContinueStmt *node) override;
    void visit(ItemStmt *node) override;


    /**
     * 设置目标地址（用于聚合类型的原地初始化优化）
     * @param address 目标内存地址
     */
    void set_target_address(const std::string &address) { target_address_ = address; }

    /**
     * 获取并清除目标地址
     * @return 目标地址，如果没有则返回空字符串
     */
    std::string take_target_address() {
        std::string addr = target_address_;
        target_address_ = "";
        return addr;
    }

  private:

    IREmitter emitter_;
    TypeMapper type_mapper_;
    ValueManager value_manager_;


    /**
     * 目标地址（用于聚合类型的原地初始化优化）
     */
    std::string target_address_;


    /**
     * 表达式结果存储
     * key: 表达式节点指针
     * value: 该表达式计算结果所在的 IR 变量名（如 "%0", "%1"）
     */
    std::map<Expr *, std::string> expr_results_;

    /**
     * 记录哪些表达式结果已经是值类型（对于聚合类型）
     * 对于聚合类型表达式，如果结果已经是值（load过），则在此集合中
     * 用于避免重复load
     */
    std::set<Expr *> loaded_aggregate_results_;


    int if_counter_ = 0;
    int while_counter_ = 0;
    int loop_counter_ = 0;


    /**
     * 循环上下文：用于 break/continue 跳转
     */
    struct LoopContext {
        std::string continue_label;
        std::string break_label;
    };

    std::vector<LoopContext> loop_stack_;


    /**
     * 标记当前基本块是否已终止（有 br/ret/unreachable）
     * 用于避免在已终止的块中添加额外的终结指令
     */
    bool current_block_terminated_ = false;

    /**
     * 当前基本块的标签名
     * 用于PHI节点记录正确的前驱块
     */
    std::string current_block_label_;

    /**
     * 标记当前函数是否使用 sret 优化
     */
    bool current_function_uses_sret_ = false;

    /**
     * 当前函数的返回类型 (IR type string)
     * 用于在 return 语句中进行类型匹配和转换
     */
    std::string current_function_return_type_str_;


    /**
     * 标记当前是否在生成左值（用于赋值目标）
     * 为true时，IndexExpr和FieldAccessExpr返回指针而不load值
     */
    bool generating_lvalue_ = false;


    /**
     * 处理顶层 Item（函数、结构体等）
     */
    void visit_item(Item *item);

    /**
     * 处理函数定义
     */
    void visit_function_decl(FnDecl *node);

    /**
     * 处理结构体定义
     */
    void visit_struct_decl(StructDecl *node);

    /**
     * 处理const常量定义
     */
    void visit_const_decl(ConstDecl *node);

    /**
     * 处理impl块
     */
    void visit_impl_block(ImplBlock *node);

    /**
     * 收集程序中所有struct定义（包括local struct）
     * 需要在生成IR前调用，确保所有类型定义在使用前声明
     */
    void collect_all_structs(Program *program);

    /**
     * 递归收集语句中的struct定义
     */
    void collect_structs_from_stmt(Stmt *stmt);


    /**
     * 获取表达式的计算结果（IR 变量名）
     * @param node 表达式节点
     * @return IR 变量名（如 "%0"）
     */
    std::string get_expr_result(Expr *node);

    /**
     * 存储表达式的计算结果
     * @param node 表达式节点
     * @param ir_var IR 变量名
     */
    void store_expr_result(Expr *node, const std::string &ir_var);

    /**
     * 开始新的基本块并更新当前块标签
     */
    void begin_block(const std::string &label);

    /**
     * 声明内置函数需要的C库函数
     */
    void emit_builtin_declarations();

    /**
     * 检查并处理内置函数调用
     * @param node 调用表达式节点
     * @param func_name 函数名
     * @param args 已计算好的参数列表 (type, value)
     * @return true如果是内置函数并已处理，false否则
     */
    bool handle_builtin_function(CallExpr *node, const std::string &func_name,
                                 const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * 将 Token 运算符转换为 LLVM IR 运算符
     * @param op Token 运算符
     * @param is_unsigned 操作数是否为无符号类型
     * @return IR 运算符字符串（如 "add", "sub", "mul"）
     */
    std::string token_to_ir_op(const Token &op, bool is_unsigned = false);

    /**
     * 将 Token 比较运算符转换为 LLVM icmp 谓词
     * @param op Token 运算符
     * @param is_unsigned 操作数是否为无符号类型
     * @return icmp 谓词（如 "eq", "ne", "slt", "sle"）
     */
    std::string token_to_icmp_pred(const Token &op, bool is_unsigned = false);

    /**
     * 处理逻辑运算符（&& 和 ||）的短路求值
     * @param node 二元表达式节点
     */
    void visit_logical_binary_expr(BinaryExpr *node);

    /**
     * 检查类型是否为有符号整数
     */
    bool is_signed_integer(Type *type);

    /**
     * 获取整数类型的位宽
     * @param kind 类型种类
     * @return 位宽（1 for bool, 32 for i32/u32, 64 for isize/usize）
     */
    int get_integer_bits(TypeKind kind);

    /**
     * 编译时求值常量表达式
     * @param expr 表达式节点
     * @param result 输出：求值结果（作为字符串）
     * @return 是否成功求值
     */
    bool evaluate_const_expr(Expr *expr, std::string &result);

    /**
     * 获取类型的大小（字节）
     * 考虑对齐和填充
     */
    size_t get_type_size(Type *type);

    /**
     * 获取类型的对齐要求（字节）
     */
    size_t get_type_alignment(Type *type);

    /**
     * 检查表达式是否为零初始化器
     * 递归检查整数字面量、结构体初始化器、数组字面量
     * @param expr 要检查的表达式
     * @return true如果是全零初始化
     */
    bool is_zero_initializer(Expr *expr);

    /**
     * 检查函数是否应该使用 sret 优化
     * 条件：函数名以 _new 结尾 且 返回大型结构体(>64字节)
     */
    bool should_use_sret_optimization(const std::string &func_name, Type *return_type);

    std::unordered_map<std::string, std::string> const_values_;

    std::unordered_map<Type *, size_t> type_size_cache_;

    std::unordered_map<std::string, int> field_index_cache_;


    /**
     * 嵌套函数队列：存储在函数体内定义的函数
     * 这些函数会在当前函数生成完成后被提升为顶层函数
     */
    std::vector<FnDecl *> nested_functions_;

    /**
     * 标记当前是否在函数体内（用于检测嵌套函数）
     */
    bool inside_function_body_ = false;

    /**
     * Local struct队列：存储在函数体内定义的struct
     * 这些struct的类型定义需要在模块顶部生成
     * 使用set避免重复收集
     */
    std::set<StructDecl *> local_structs_set_;
};
