#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declarations for all AST nodes
class Type;
class Symbol;

struct Node;
struct Expr;
struct Stmt;
struct Item;
struct TypeNode;
struct Pattern;
struct Program;

// Expression forward declarations
struct LiteralExpr;
struct ArrayLiteralExpr;
struct ArrayInitializerExpr;
struct VariableExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CallExpr;
struct IfExpr;
struct LoopExpr;
struct WhileExpr;
struct IndexExpr;
struct FieldAccessExpr;
struct AssignmentExpr;
struct CompoundAssignmentExpr;
struct ReferenceExpr;
struct UnderscoreExpr;
struct AsExpr;
struct StructInitializerExpr;
struct UnitExpr;
struct GroupingExpr;
struct TupleExpr;
struct MatchExpr;
struct PathExpr;
struct BlockExpr;

// Statement forward declarations
struct BlockStmt;
struct ExprStmt;
struct LetStmt;
struct ReturnStmt;
struct BreakStmt;
struct ContinueStmt;
struct ItemStmt;

// Type node forward declarations
struct TypeNameNode;
struct ArrayTypeNode;
struct UnitTypeNode;
struct TupleTypeNode;
struct PathTypeNode;
struct RawPointerTypeNode;
struct ReferenceTypeNode;
struct SliceTypeNode;
struct SelfTypeNode;

// Item forward declarations
struct FnDecl;
struct StructDecl;
struct ConstDecl;
struct EnumDecl;
struct EnumVariant;
struct ModDecl;
struct TraitDecl;
struct ImplBlock;

// Pattern forward declarations
struct IdentifierPattern;
struct WildcardPattern;
struct LiteralPattern;
struct TuplePattern;
struct SlicePattern;
struct StructPattern;
struct RestPattern;

// Other forward declarations
struct MatchArm;
struct FnParam;
struct Field;
struct FieldInitializer;
struct StructPatternField;

// Template-based expression visitor for different return types
template <typename R> class ExprVisitor {
  public:
    virtual ~ExprVisitor() = default;

    // Expression visitors with template return type R
    virtual R visit(LiteralExpr *node) = 0;
    virtual R visit(ArrayLiteralExpr *node) = 0;
    virtual R visit(ArrayInitializerExpr *node) = 0;
    virtual R visit(VariableExpr *node) = 0;
    virtual R visit(UnaryExpr *node) = 0;
    virtual R visit(BinaryExpr *node) = 0;
    virtual R visit(CallExpr *node) = 0;
    virtual R visit(IfExpr *node) = 0;
    virtual R visit(LoopExpr *node) = 0;
    virtual R visit(WhileExpr *node) = 0;
    virtual R visit(IndexExpr *node) = 0;
    virtual R visit(FieldAccessExpr *node) = 0;
    virtual R visit(AssignmentExpr *node) = 0;
    virtual R visit(CompoundAssignmentExpr *node) = 0;
    virtual R visit(ReferenceExpr *node) = 0;
    virtual R visit(UnderscoreExpr *node) = 0;
    virtual R visit(StructInitializerExpr *node) = 0;
    virtual R visit(UnitExpr *node) = 0;
    virtual R visit(GroupingExpr *node) = 0;
    virtual R visit(TupleExpr *node) = 0;
    virtual R visit(AsExpr *node) = 0;
    virtual R visit(MatchExpr *node) = 0;
    virtual R visit(PathExpr *node) = 0;
    virtual R visit(BlockExpr *node) = 0;
};

// Type aliases for common visitor types
using NameResolutionVisitor_t = ExprVisitor<std::shared_ptr<Symbol>>;
using TypeCheckVisitor_t = ExprVisitor<std::shared_ptr<Type>>;
using ConstEvaluator_t = ExprVisitor<std::optional<long long>>;

class StmtVisitor {
  public:
    virtual ~StmtVisitor() = default;

    virtual void visit(BlockStmt *node) = 0;
    virtual void visit(ExprStmt *node) = 0;
    virtual void visit(LetStmt *node) = 0;
    virtual void visit(ReturnStmt *node) = 0;
    virtual void visit(BreakStmt *node) = 0;
    virtual void visit(ContinueStmt *node) = 0;
    virtual void visit(ItemStmt *node) = 0;
};

class ItemVisitor {
  public:
    virtual ~ItemVisitor() = default;

    virtual void visit(FnDecl *node) = 0;
    virtual void visit(StructDecl *node) = 0;
    virtual void visit(ConstDecl *node) = 0;
    virtual void visit(EnumDecl *node) = 0;
    virtual void visit(ModDecl *node) = 0;
    virtual void visit(TraitDecl *node) = 0;
    virtual void visit(ImplBlock *node) = 0;
};

class TypeVisitor {
  public:
    virtual ~TypeVisitor() = default;

    virtual void visit(TypeNameNode *node) = 0;
    virtual void visit(ArrayTypeNode *node) = 0;
    virtual void visit(UnitTypeNode *node) = 0;
    virtual void visit(TupleTypeNode *node) = 0;
    virtual void visit(PathTypeNode *node) = 0;
    virtual void visit(RawPointerTypeNode *node) = 0;
    virtual void visit(ReferenceTypeNode *node) = 0;
    virtual void visit(SliceTypeNode *node) = 0;
    virtual void visit(SelfTypeNode *node) = 0;
};

class PatternVisitor {
  public:
    virtual ~PatternVisitor() = default;

    virtual void visit(IdentifierPattern *node) = 0;
    virtual void visit(WildcardPattern *node) = 0;
    virtual void visit(LiteralPattern *node) = 0;
    virtual void visit(TuplePattern *node) = 0;
    virtual void visit(SlicePattern *node) = 0;
    virtual void visit(StructPattern *node) = 0;
    virtual void visit(RestPattern *node) = 0;
};
