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
 * IRGenerator - LLVM IR Generator
 *
 * Core responsibilities:
 * 1. Traverse AST to generate LLVM IR text
 * 2. Coordinate IREmitter, TypeMapper, ValueManager
 * 3. Handle expressions, statements, function definitions
 *
 * Design principles:
 * - Use visitor pattern to traverse AST
 * - Expression results are stored in expr_results_, not passed by return value
 * - Directly use the type field in AST nodes to get type information
 * - Do not use LLVM C++ API, generate plain text IR
 */
class IRGenerator : public ExprVisitor<void>, public StmtVisitor {
  public:
    /**
     * Constructor
     * @param builtin_types Reference to built-in types (from semantic analyzer)
     */
    explicit IRGenerator(BuiltinTypes &builtin_types);

    /**
     * Generate IR for complete program
     * @param program AST root node
     * @return Generated LLVM IR text
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
     * Set target address (for in-place initialization optimization of aggregate types)
     * @param address Target memory address
     */
    void set_target_address(const std::string &address) { target_address_ = address; }

    /**
     * Get and clear target address
     * @return Target address, or empty string if none
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
     * Target address (for in-place initialization optimization of aggregate types)
     */
    std::string target_address_;

    /**
     * Expression result storage
     * key: Expression node pointer
     * value: IR variable name where the expression result is stored (e.g., "%0", "%1")
     */
    std::map<Expr *, std::string> expr_results_;

    /**
     * Records which expression results are already value types (for aggregate types)
     * For aggregate type expressions, if the result is already a value (loaded), it's in this set
     * Used to avoid redundant loads
     */
    std::set<Expr *> loaded_aggregate_results_;

    int if_counter_ = 0;
    int while_counter_ = 0;
    int loop_counter_ = 0;

    /**
     * Loop context: for break/continue jumps
     */
    struct LoopContext {
        std::string continue_label;
        std::string break_label;
    };

    std::vector<LoopContext> loop_stack_;

    /**
     * Marks whether current basic block is terminated (has br/ret/unreachable)
     * Used to avoid adding extra terminator instructions in terminated blocks
     */
    bool current_block_terminated_ = false;

    /**
     * Current basic block label name
     * Used to record correct predecessor block for PHI nodes
     */
    std::string current_block_label_;

    /**
     * Marks whether current function uses sret optimization
     */
    bool current_function_uses_sret_ = false;

    /**
     * Current function's return type (IR type string)
     * Used for type matching and conversion in return statements
     */
    std::string current_function_return_type_str_;

    /**
     * Marks whether currently generating lvalue (for assignment target)
     * When true, IndexExpr and FieldAccessExpr return pointer instead of loading value
     */
    bool generating_lvalue_ = false;

    /**
     * Process top-level Item (functions, structs, etc.)
     */
    void visit_item(Item *item);

    /**
     * Process function definition
     */
    void visit_function_decl(FnDecl *node);

    /**
     * Process struct definition
     */
    void visit_struct_decl(StructDecl *node);

    /**
     * Process const constant definition
     */
    void visit_const_decl(ConstDecl *node);

    /**
     * Process impl block
     */
    void visit_impl_block(ImplBlock *node);

    /**
     * Collect all struct definitions in program (including local structs)
     * Should be called before generating IR to ensure all type definitions are declared before use
     */
    void collect_all_structs(Program *program);

    /**
     * Recursively collect struct definitions from statements
     */
    void collect_structs_from_stmt(Stmt *stmt);

    /**
     * Get expression computation result (IR variable name)
     * @param node Expression node
     * @return IR variable name (e.g., "%0")
     */
    std::string get_expr_result(Expr *node);

    /**
     * Store expression computation result
     * @param node Expression node
     * @param ir_var IR variable name
     */
    void store_expr_result(Expr *node, const std::string &ir_var);

    /**
     * Begin new basic block and update current block label
     */
    void begin_block(const std::string &label);

    /**
     * Declare C library functions needed by built-in functions
     */
    void emit_builtin_declarations();

    /**
     * Check and handle built-in function call
     * @param node Call expression node
     * @param func_name Function name
     * @param args Pre-computed argument list (type, value)
     * @return true if it's a built-in function and handled, false otherwise
     */
    bool handle_builtin_function(CallExpr *node, const std::string &func_name,
                                 const std::vector<std::pair<std::string, std::string>> &args);

    /**
     * Convert Token operator to LLVM IR operator
     * @param op Token operator
     * @param is_unsigned Whether operands are unsigned type
     * @return IR operator string (e.g., "add", "sub", "mul")
     */
    std::string token_to_ir_op(const Token &op, bool is_unsigned = false);

    /**
     * Convert Token comparison operator to LLVM icmp predicate
     * @param op Token operator
     * @param is_unsigned Whether operands are unsigned type
     * @return icmp predicate (e.g., "eq", "ne", "slt", "sle")
     */
    std::string token_to_icmp_pred(const Token &op, bool is_unsigned = false);

    /**
     * Handle short-circuit evaluation for logical operators (&& and ||)
     * @param node Binary expression node
     */
    void visit_logical_binary_expr(BinaryExpr *node);

    /**
     * Check if type is signed integer
     */
    bool is_signed_integer(Type *type);

    /**
     * Get bit width of integer type
     * @param kind Type kind
     * @return Bit width (1 for bool, 32 for i32/u32, 64 for isize/usize)
     */
    int get_integer_bits(TypeKind kind);

    /**
     * Compile-time evaluate constant expression
     * @param expr Expression node
     * @param result Output: evaluation result (as string)
     * @return Whether evaluation succeeded
     */
    bool evaluate_const_expr(Expr *expr, std::string &result);

    /**
     * Get type size (in bytes)
     * Considers alignment and padding
     */
    size_t get_type_size(Type *type);

    /**
     * Get type alignment requirement (in bytes)
     */
    size_t get_type_alignment(Type *type);

    /**
     * Check if expression is a zero initializer
     * Recursively checks integer literals, struct initializers, array literals
     * @param expr Expression to check
     * @return true if it's an all-zero initialization
     */
    bool is_zero_initializer(Expr *expr);

    /**
     * Check if function should use sret optimization
     * Condition: function name ends with _new AND returns large struct (>64 bytes)
     */
    bool should_use_sret_optimization(const std::string &func_name, Type *return_type);

    std::unordered_map<std::string, std::string> const_values_;

    std::unordered_map<Type *, size_t> type_size_cache_;

    std::unordered_map<std::string, int> field_index_cache_;

    /**
     * Nested function queue: stores functions defined inside function body
     * These functions will be hoisted to top-level after current function generation completes
     */
    std::vector<FnDecl *> nested_functions_;

    /**
     * Marks whether currently inside function body (for detecting nested functions)
     */
    bool inside_function_body_ = false;

    /**
     * Local struct queue: stores structs defined inside function body
     * Type definitions for these structs need to be generated at module top
     * Uses set to avoid duplicate collection
     */
    std::set<StructDecl *> local_structs_set_;
};
