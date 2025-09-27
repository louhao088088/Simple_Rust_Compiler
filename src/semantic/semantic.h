// semantic.h

#pragma once

#include "../ast/ast.h"
#include "../error/error.h"
#include "../tool/number.h"

#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;

// Type system base class

enum class TypeKind {
    I32,
    U32,
    ISIZE,
    USIZE,
    ANY_INTEGER,
    STR,
    STRING,
    RSTRING,
    CSTRING,
    RCSTRING,
    CHAR,
    BOOL,
    ARRAY,
    STRUCT,
    UNIT,
    FUNCTION,
    REFERENCE,
    NEVER,
    UNKNOWN,
};
class SymbolTable;

struct BuiltinTypes {
    std::shared_ptr<Type> i32_type;
    std::shared_ptr<Type> u32_type;
    std::shared_ptr<Type> isize_type;
    std::shared_ptr<Type> usize_type;
    std::shared_ptr<Type> string_type;
    std::shared_ptr<Type> str_type;
    std::shared_ptr<Type> bool_type;
    std::shared_ptr<Type> any_integer_type;
    std::shared_ptr<Type> char_type;
    std::shared_ptr<Type> unit_type;
};

struct Type {
    TypeKind kind;
    std::shared_ptr<SymbolTable> members = std::make_shared<SymbolTable>();
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
    virtual bool equals(const Type *other) const = 0;
};
bool is_concrete_integer(TypeKind kind);
bool is_any_integer_type(TypeKind kind);

struct PrimitiveType : public Type {
    PrimitiveType(TypeKind primitive_kind) { this->kind = primitive_kind; }

    std::string to_string() const override {
        switch (kind) {
        case TypeKind::I32:
            return "i32";
        case TypeKind::U32:
            return "u32";
        case TypeKind::ISIZE:
            return "isize";
        case TypeKind::USIZE:
            return "usize";
        case TypeKind::ANY_INTEGER:
            return "anyint";
        case TypeKind::BOOL:
            return "bool";
        case TypeKind::STR:
            return "str";
        case TypeKind::STRING:
            return "string";
        case TypeKind::RSTRING:
            return "rstring";
        case TypeKind::CSTRING:
            return "cstring";
        case TypeKind::RCSTRING:
            return "rcstring";
        case TypeKind::CHAR:
            return "char";
        case TypeKind::UNIT:
            return "()";
        case TypeKind::NEVER:
            return "!";
        default:
            return "unknown";
        }
    }

    bool equals(const Type *other) const override {
        if (other == nullptr)
            return false;
        TypeKind other_kind = other->kind;

        if (this->kind == TypeKind::NEVER || other_kind == TypeKind::NEVER) {
            return true;
        }

        if (dynamic_cast<const PrimitiveType *>(other) == nullptr) {
            return false;
        }

        if (this->kind == TypeKind::ANY_INTEGER) {
            return is_concrete_integer(other_kind) || other_kind == TypeKind::ANY_INTEGER;
        }

        if (other_kind == TypeKind::ANY_INTEGER) {
            return is_concrete_integer(this->kind);
        }

        return this->kind == other_kind;
    }
};

struct ArrayType : public Type {
    std::shared_ptr<Type> element_type; // T
    size_t size;                        // N

    ArrayType(std::shared_ptr<Type> et, size_t sz) : element_type(std::move(et)), size(sz) {
        this->kind = TypeKind::ARRAY;
    }

    std::string to_string() const override {
        return "[" + element_type->to_string() + "; " + std::to_string(size) + "]";
    }

    bool equals(const Type *other) const override {
        if (other->kind == TypeKind::NEVER) {
            return true;
        }
        if (auto *other_array = dynamic_cast<const ArrayType *>(other)) {
            return size == other_array->size &&
                   element_type->equals(other_array->element_type.get());
        }
        return false;
    }
};

struct StructType : public Type {
    std::string name;
    std::map<std::string, std::shared_ptr<Type>> fields;
    std::weak_ptr<Symbol> symbol;

    StructType(std::string name, std::weak_ptr<Symbol> symbol)
        : name(std::move(name)), symbol(symbol) {
        this->kind = TypeKind::STRUCT;
    }

    std::string to_string() const override { return name; }

    bool equals(const Type *other) const override {
        if (other->kind == TypeKind::NEVER) {
            return true;
        }
        if (auto *other_struct = dynamic_cast<const StructType *>(other)) {
            return name == other_struct->name;
        }
        return false;
    }
};

struct UnitType : public Type {
    UnitType() { this->kind = TypeKind::UNIT; }
    std::string to_string() const override { return "()"; }
    bool equals(const Type *other) const override {
        return other->kind == TypeKind::UNIT || other->kind == TypeKind::NEVER;
    }
};

struct NeverType : public Type {
    NeverType() { this->kind = TypeKind::NEVER; }
    std::string to_string() const override { return "!"; }
    bool equals(const Type *other) const override { return true; }
};

struct FunctionType : public Type {
    std::shared_ptr<Type> return_type;
    std::vector<std::shared_ptr<Type>> param_types;

    FunctionType(std::shared_ptr<Type> ret_type, std::vector<std::shared_ptr<Type>> p_types)
        : return_type(std::move(ret_type)) {
        param_types.resize(p_types.size());
        for (size_t i = 0; i < p_types.size(); ++i) {
            param_types[i] = std::move(p_types[i]);
        }
        this->kind = TypeKind::FUNCTION;
    }

    std::string to_string() const override {
        std::string param_str;
        for (const auto &param : param_types) {
            if (!param_str.empty()) {
                param_str += ", ";
            }
            param_str += param->to_string();
        }
        return "fn(" + param_str + ") -> " + return_type->to_string();
    }

    bool equals(const Type *other) const override {
        if (auto *other_fn = dynamic_cast<const FunctionType *>(other)) {
            if (!return_type->equals(other_fn->return_type.get()))
                return false;
            if (param_types.size() != other_fn->param_types.size())
                return false;
            for (size_t i = 0; i < param_types.size(); ++i) {
                if (!param_types[i]->equals(other_fn->param_types[i].get()))
                    return false;
            }
            return true;
        }
        return false;
    }
};

struct ReferenceType : public Type {
    std::shared_ptr<Type> referenced_type;
    bool is_mutable;

    ReferenceType(std::shared_ptr<Type> ref_type, bool is_mut = false)
        : referenced_type(std::move(ref_type)), is_mutable(is_mut) {
        this->kind = TypeKind::REFERENCE;
    }

    std::string to_string() const override {
        if (is_mutable) {
            return "&mut " + referenced_type->to_string();
        } else {
            return "&" + referenced_type->to_string();
        }
    }

    bool equals(const Type *other) const override {
        if (other->kind == TypeKind::NEVER) {
            return true;
        }
        if (auto *other_ref = dynamic_cast<const ReferenceType *>(other)) {

            return is_mutable == other_ref->is_mutable &&
                   referenced_type->equals(other_ref->referenced_type.get());
        }
        return false;
    }
};

class SymbolTable;
class NameResolutionVisitor;

class TypeResolver : public TypeVisitor {
  public:
    TypeResolver(NameResolutionVisitor &resolver, SymbolTable &symbols, ErrorReporter &reporter);

    std::shared_ptr<Type> resolve(TypeNode *node);

    void visit(TypeNameNode *node) override;
    void visit(ArrayTypeNode *node) override;
    void visit(UnitTypeNode *node) override;
    void visit(TupleTypeNode *node) override;
    void visit(PathTypeNode *node) override;
    void visit(RawPointerTypeNode *node) override;
    void visit(ReferenceTypeNode *node) override;
    void visit(SliceTypeNode *node) override;
    void visit(SelfTypeNode *node) override;

  private:
    NameResolutionVisitor &name_resolver_;
    SymbolTable &symbol_table_;
    std::shared_ptr<Type> resolved_type_;
    ErrorReporter &error_reporter_;
};

class Symbol {
  public:
    enum Kind { VARIABLE, FUNCTION, TYPE, MODULE, VARIANT, CONSTANT };

    std::string name;
    Kind kind;
    std::shared_ptr<Type> type;
    std::shared_ptr<SymbolTable> members;
    std::shared_ptr<Symbol> aliased_symbol;

    bool is_mutable;
    bool is_builtin;

    ConstDecl *const_decl_node = nullptr;

    Symbol(std::string name, Kind kind, std::shared_ptr<Type> type = nullptr)
        : name(std::move(name)), kind(kind), type(std::move(type)),
          members(std::make_shared<SymbolTable>()), aliased_symbol(nullptr), is_mutable(false),
          is_builtin(false) {}

    virtual ~Symbol() = default;
};

class SymbolTable {
  public:
    struct Scope {
        std::unordered_map<std::string, std::shared_ptr<Symbol>> value_symbols; // fn/let/const
        std::unordered_map<std::string, std::shared_ptr<Symbol>> type_symbols;  // struct/enum/type
    };

    SymbolTable() { enter_scope(); }
    void enter_scope();
    void exit_scope();

    bool define_value(const std::string &name, std::shared_ptr<Symbol> symbol);
    bool define_variable(const std::string &name, std::shared_ptr<Symbol> symbol,
                         bool allow_shadow);
    std::shared_ptr<Symbol> lookup_value(const std::string &name);
    bool define_type(const std::string &name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup_type(const std::string &name);

    bool define(const std::string &name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup(const std::string &name);

  private:
    std::vector<Scope> scopes_;
};

class ConstEvaluator : public ExprVisitor<std::optional<long long>> {
  public:
    ConstEvaluator(SymbolTable &symbol_table, ErrorReporter &error_reporter)
        : symbol_table_(symbol_table), error_reporter_(error_reporter) {}

    std::optional<long long> evaluate(Expr *expr) {
        if (!expr)
            return std::nullopt;
        return expr->accept(this);
    }

    std::optional<long long> visit(LiteralExpr *node) override;
    std::optional<long long> visit(VariableExpr *node) override;
    std::optional<long long> visit(BinaryExpr *node) override;
    std::optional<long long> visit(UnaryExpr *node) override;
    std::optional<long long> visit(GroupingExpr *node) override;

    // Default implementations for non-constant expressions
    std::optional<long long> visit(ArrayLiteralExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(ArrayInitializerExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(CallExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(IfExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(LoopExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(WhileExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(IndexExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(FieldAccessExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(AssignmentExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(CompoundAssignmentExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(ReferenceExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(UnderscoreExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(StructInitializerExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(UnitExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(TupleExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(AsExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(MatchExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(PathExpr *node) override { return std::nullopt; }
    std::optional<long long> visit(BlockExpr *node) override { return std::nullopt; }

  private:
    SymbolTable &symbol_table_;
    ErrorReporter &error_reporter_;
};

class NameResolutionVisitor : public ExprVisitor<std::shared_ptr<Symbol>>,
                              public StmtVisitor,
                              public ItemVisitor,
                              public TypeVisitor,
                              public PatternVisitor {
  public:
    NameResolutionVisitor(ErrorReporter &error_reporter);
    SymbolTable &get_global_symbol_table() { return symbol_table_; }

    void resolve(Program *ast);

    // Expression visitors
    std::shared_ptr<Symbol> visit(LiteralExpr *node) override;
    std::shared_ptr<Symbol> visit(ArrayLiteralExpr *node) override;
    std::shared_ptr<Symbol> visit(ArrayInitializerExpr *node) override;
    std::shared_ptr<Symbol> visit(VariableExpr *node) override;
    std::shared_ptr<Symbol> visit(UnaryExpr *node) override;
    std::shared_ptr<Symbol> visit(BinaryExpr *node) override;
    std::shared_ptr<Symbol> visit(CallExpr *node) override;
    std::shared_ptr<Symbol> visit(IfExpr *node) override;
    std::shared_ptr<Symbol> visit(LoopExpr *node) override;
    std::shared_ptr<Symbol> visit(WhileExpr *node) override;
    std::shared_ptr<Symbol> visit(IndexExpr *node) override;
    std::shared_ptr<Symbol> visit(FieldAccessExpr *node) override;
    std::shared_ptr<Symbol> visit(AssignmentExpr *node) override;
    std::shared_ptr<Symbol> visit(CompoundAssignmentExpr *node) override;
    std::shared_ptr<Symbol> visit(ReferenceExpr *node) override;
    std::shared_ptr<Symbol> visit(UnderscoreExpr *node) override;
    std::shared_ptr<Symbol> visit(StructInitializerExpr *node) override;
    std::shared_ptr<Symbol> visit(UnitExpr *node) override;
    std::shared_ptr<Symbol> visit(GroupingExpr *node) override;
    std::shared_ptr<Symbol> visit(TupleExpr *node) override;
    std::shared_ptr<Symbol> visit(AsExpr *node) override;
    std::shared_ptr<Symbol> visit(MatchExpr *node) override;
    std::shared_ptr<Symbol> visit(PathExpr *node) override;
    std::shared_ptr<Symbol> visit(BlockExpr *node) override;

    // Statement visitors
    void visit(BlockStmt *node) override;
    void visit(ExprStmt *node) override;
    void visit(LetStmt *node) override;
    void visit(ReturnStmt *node) override;
    void visit(BreakStmt *node) override;
    void visit(ContinueStmt *node) override;
    void visit(ItemStmt *node) override;

    // Type node visitors
    void visit(TypeNameNode *node) override;
    void visit(ArrayTypeNode *node) override;
    void visit(UnitTypeNode *node) override;
    void visit(TupleTypeNode *node) override;
    void visit(PathTypeNode *node) override;
    void visit(RawPointerTypeNode *node) override;
    void visit(ReferenceTypeNode *node) override;
    void visit(SliceTypeNode *node) override;
    void visit(SelfTypeNode *node) override;

    // Item visitors
    void visit(FnDecl *node) override;
    void visit(StructDecl *node) override;
    void visit(ConstDecl *node) override;
    void visit(EnumDecl *node) override;
    void visit(ModDecl *node) override;
    void visit(TraitDecl *node) override;
    void visit(ImplBlock *node) override;

    // Pattern visitors
    void visit(IdentifierPattern *node) override;
    void visit(WildcardPattern *node) override;
    void visit(LiteralPattern *node) override;
    void visit(TuplePattern *node) override;
    void visit(SlicePattern *node) override;
    void visit(StructPattern *node) override;
    void visit(RestPattern *node) override;
    void visit(ReferencePattern *node) override;

  private:
    SymbolTable symbol_table_;
    ErrorReporter &error_reporter_;
    TypeResolver type_resolver_;
    std::shared_ptr<Type> current_type_ = nullptr;

    void declare_function(FnDecl *node);
    void define_function_body(FnDecl *node);

    void declare_struct(StructDecl *node);
    void define_struct_body(StructDecl *node);
    void declare_impl_method(ImplBlock *node);
};

// Type check visitor
class TypeCheckVisitor : public ExprVisitor<std::shared_ptr<Symbol>>,
                         public StmtVisitor,
                         public ItemVisitor,
                         public TypeVisitor,
                         public PatternVisitor {
  public:
    TypeCheckVisitor(SymbolTable &symbol_table, BuiltinTypes &builtin_types,
                     ErrorReporter &error_reporter);

    // Expression visitors
    std::shared_ptr<Symbol> visit(LiteralExpr *node) override;
    std::shared_ptr<Symbol> visit(ArrayLiteralExpr *node) override;
    std::shared_ptr<Symbol> visit(ArrayInitializerExpr *node) override;
    std::shared_ptr<Symbol> visit(VariableExpr *node) override;
    std::shared_ptr<Symbol> visit(UnaryExpr *node) override;
    std::shared_ptr<Symbol> visit(BinaryExpr *node) override;
    std::shared_ptr<Symbol> visit(CallExpr *node) override;
    std::shared_ptr<Symbol> visit(IfExpr *node) override;
    std::shared_ptr<Symbol> visit(LoopExpr *node) override;
    std::shared_ptr<Symbol> visit(WhileExpr *node) override;
    std::shared_ptr<Symbol> visit(IndexExpr *node) override;
    std::shared_ptr<Symbol> visit(FieldAccessExpr *node) override;
    std::shared_ptr<Symbol> visit(AssignmentExpr *node) override;
    std::shared_ptr<Symbol> visit(CompoundAssignmentExpr *node) override;
    std::shared_ptr<Symbol> visit(ReferenceExpr *node) override;
    std::shared_ptr<Symbol> visit(UnderscoreExpr *node) override;
    std::shared_ptr<Symbol> visit(StructInitializerExpr *node) override;
    std::shared_ptr<Symbol> visit(UnitExpr *node) override;
    std::shared_ptr<Symbol> visit(GroupingExpr *node) override;
    std::shared_ptr<Symbol> visit(TupleExpr *node) override;
    std::shared_ptr<Symbol> visit(AsExpr *node) override;
    std::shared_ptr<Symbol> visit(MatchExpr *node) override;
    std::shared_ptr<Symbol> visit(PathExpr *node) override;
    std::shared_ptr<Symbol> visit(BlockExpr *node) override;

    // Statement visitors
    void visit(BlockStmt *node) override;
    void visit(ExprStmt *node) override;
    void visit(LetStmt *node) override;
    void visit(ReturnStmt *node) override;
    void visit(BreakStmt *node) override;
    void visit(ContinueStmt *node) override;
    void visit(ItemStmt *node) override;

    // Type node visitors
    void visit(TypeNameNode *node) override;
    void visit(ArrayTypeNode *node) override;
    void visit(UnitTypeNode *node) override;
    void visit(TupleTypeNode *node) override;
    void visit(PathTypeNode *node) override;
    void visit(RawPointerTypeNode *node) override;
    void visit(ReferenceTypeNode *node) override;
    void visit(SliceTypeNode *node) override;
    void visit(SelfTypeNode *node) override;

    // Item visitors
    void visit(FnDecl *node) override;
    void visit(StructDecl *node) override;
    void visit(ConstDecl *node) override;
    void visit(EnumDecl *node) override;
    void visit(ModDecl *node) override;
    void visit(TraitDecl *node) override;
    void visit(ImplBlock *node) override;

    // Pattern visitors
    void visit(IdentifierPattern *node) override;
    void visit(WildcardPattern *node) override;
    void visit(LiteralPattern *node) override;
    void visit(TuplePattern *node) override;
    void visit(SlicePattern *node) override;
    void visit(StructPattern *node) override;
    void visit(RestPattern *node) override;
    void visit(ReferencePattern *node) override;

  private:
    SymbolTable &symbol_table_;
    ErrorReporter &error_reporter_;
    BuiltinTypes &builtin_types_;
    std::shared_ptr<Type> current_return_type_ = nullptr;
    Symbol *current_function_symbol_ = nullptr;
    int loop_depth_ = 0;
    std::vector<std::shared_ptr<Type>> breakable_expr_type_stack_;

    void check_main_for_early_exit(BlockStmt *body);
};

void Semantic(std::shared_ptr<Program> &ast, ErrorReporter &error_reporter);

std::optional<std::string> get_name_from_expr(Expr *expr);