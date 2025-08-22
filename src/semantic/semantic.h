// semantic.h

#pragma once

#include "../ast/ast.h"
#include "../error/error.h"

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::vector;

struct Number {
    long long value;
    bool is_signed;
};

// Type system base class

enum class TypeKind {
    INTEGER,
    UNSIGNED_INTEGER,
    STRING,
    RSTRING,
    CSTRING,
    RCSTRING,
    CHAR,
    BOOL,
    ARRAY,
    STRUCT,
    UNIT,
    UNKNOWN,
};

struct Type {
    TypeKind kind;
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
    virtual bool equals(const Type *other) const = 0;
};

struct PrimitiveType : public Type {
    PrimitiveType(TypeKind primitive_kind) { this->kind = primitive_kind; }

    std::string to_string() const override {
        switch (kind) {
        case TypeKind::INTEGER:
            return "i32";
        case TypeKind::UNSIGNED_INTEGER:
            return "u32";
        case TypeKind::BOOL:
            return "bool";
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
        default:
            return "unknown";
        }
    }

    bool equals(const Type *other) const override { return kind == other->kind; }
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
        if (auto *other_struct = dynamic_cast<const StructType *>(other)) {
            return name == other_struct->name;
        }
        return false;
    }
};

struct UnitType : public Type {
    UnitType() { this->kind = TypeKind::UNIT; }
    std::string to_string() const override { return "()"; }
    bool equals(const Type *other) const override { return other->kind == TypeKind::UNIT; }
};

class SymbolTable;
class Symbol {
  public:
    enum Kind { VARIABLE, FUNCTION, TYPE, MODULE, VARIANT, CONSTANT };

    std::string name;
    Kind kind;
    std::shared_ptr<Type> type;
    std::shared_ptr<SymbolTable> members;
    std::shared_ptr<Symbol> aliased_symbol;

    Symbol(std::string name, Kind kind, std::shared_ptr<Type> type = nullptr)
        : name(std::move(name)), kind(kind), type(std::move(type)),
          members(std::make_shared<SymbolTable>()), aliased_symbol(nullptr) {}

    virtual ~Symbol() = default;
};

class SymbolTable {
  public:
    void enter_scope();
    void exit_scope();
    bool define(const std::string &name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup(const std::string &name);

  private:
    std::vector<std::unordered_map<std::string, std::shared_ptr<Symbol>>> scopes_;
};

// Name resolution visitor
class NameResolutionVisitor : public ExprVisitor,
                              public StmtVisitor,
                              public ItemVisitor,
                              public TypeVisitor,
                              public PatternVisitor,
                              public OtherVisitor,
                              public ProgramVisitor {
  public:
    NameResolutionVisitor(ErrorReporter &error_reporter);

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

    // Other visitors
    void visit(EnumVariant *node) override;
    void visit(MatchArm *node) override;

    void visit(Program *node) override;

  private:
    SymbolTable symbol_table_;
    ErrorReporter &error_reporter_;
};

// Type check visitor
class TypeCheckVisitor : public ExprVisitor,
                         public StmtVisitor,
                         public ItemVisitor,
                         public TypeVisitor,
                         public PatternVisitor,
                         public OtherVisitor,
                         public ProgramVisitor {
  public:
    TypeCheckVisitor(ErrorReporter &error_reporter);

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

    // Other visitors
    void visit(EnumVariant *node) override;
    void visit(MatchArm *node) override;

    void visit(Program *node) override;

  private:
    ErrorReporter &error_reporter_;
    std::shared_ptr<Type> current_return_type_;
};