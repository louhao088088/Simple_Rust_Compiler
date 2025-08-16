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
class Type {
  public:
    virtual ~Type() = default;
    virtual std::string to_string() const = 0;
    virtual bool equals(const Type *other) const = 0;
};

// Symbol class for symbol table
class Symbol {
  public:
    enum Kind { VARIABLE, FUNCTION, TYPE };

    std::string name;
    Kind kind;
    std::shared_ptr<Type> type;

    Symbol(std::string name, Kind kind, std::shared_ptr<Type> type = nullptr)
        : name(std::move(name)), kind(kind), type(std::move(type)) {}

    virtual ~Symbol() = default;
};

// Symbol table for scope management
class SymbolTable {
  public:
    void enter_scope();
    void exit_scope();
    void define(const std::string &name, std::shared_ptr<Symbol> symbol);
    std::shared_ptr<Symbol> lookup(const std::string &name);

  private:
    std::vector<std::map<std::string, std::shared_ptr<Symbol>>> scopes_;
};

// Name resolution visitor
class NameResolutionVisitor : public Visitor {
  public:
    NameResolutionVisitor(ErrorReporter &error_reporter);

    // Expression visitors
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

    // Statement visitors
    void visit(BlockStmt *node) override;
    void visit(ExprStmt *node) override;
    void visit(LetStmt *node) override;
    void visit(ReturnStmt *node) override;
    void visit(BreakStmt *node) override;
    void visit(ContinueStmt *node) override;

    // Type node visitors
    void visit(TypeNameNode *node) override;
    void visit(ArrayTypeNode *node) override;
    void visit(UnitTypeNode *node) override;
    void visit(TupleTypeNode *node) override;

    // Item visitors
    void visit(FnDecl *node) override;
    void visit(Program *node) override;

    // Pattern visitors
    void visit(IdentifierPattern *node) override;
    void visit(WildcardPattern *node) override;
    void visit(LiteralPattern *node) override;
    void visit(TuplePattern *node) override;
    void visit(SlicePattern *node) override;
    void visit(StructPattern *node) override;
    void visit(RestPattern *node) override;

  private:
    SymbolTable symbol_table_;
    ErrorReporter &error_reporter_;
};

// Type check visitor
class TypeCheckVisitor : public Visitor {
  public:
    TypeCheckVisitor(ErrorReporter &error_reporter);

    // Expression visitors
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

    // Statement visitors
    void visit(BlockStmt *node) override;
    void visit(ExprStmt *node) override;
    void visit(LetStmt *node) override;
    void visit(ReturnStmt *node) override;
    void visit(BreakStmt *node) override;
    void visit(ContinueStmt *node) override;

    // Type node visitors
    void visit(TypeNameNode *node) override;
    void visit(ArrayTypeNode *node) override;
    void visit(UnitTypeNode *node) override;
    void visit(TupleTypeNode *node) override;

    // Item visitors
    void visit(FnDecl *node) override;
    void visit(Program *node) override;

    // Pattern visitors
    void visit(IdentifierPattern *node) override;
    void visit(WildcardPattern *node) override;
    void visit(LiteralPattern *node) override;
    void visit(TuplePattern *node) override;
    void visit(SlicePattern *node) override;
    void visit(StructPattern *node) override;
    void visit(RestPattern *node) override;

  private:
    ErrorReporter &error_reporter_;
    std::shared_ptr<Type> current_return_type_;
};