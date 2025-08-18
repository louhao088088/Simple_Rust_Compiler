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
struct RangeExpr;

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

class ExprVisitor {
  public:
    virtual ~ExprVisitor() = default;

    // Expression visitors
    virtual std::shared_ptr<Symbol> visit(LiteralExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(ArrayLiteralExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(ArrayInitializerExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(VariableExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(UnaryExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(BinaryExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(CallExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(IfExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(LoopExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(WhileExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(IndexExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(FieldAccessExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(AssignmentExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(CompoundAssignmentExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(ReferenceExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(UnderscoreExpr *node) = 0;
    virtual std::shared_ptr<Symbol> visit(StructInitializerExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(UnitExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(GroupingExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(TupleExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(AsExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(MatchExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(PathExpr *node) { return nullptr; }
    virtual std::shared_ptr<Symbol> visit(RangeExpr *node) { return nullptr; }
};

class StmtVisitor {
  public:
    virtual ~StmtVisitor() = default;

    // Statement visitors
    virtual void visit(BlockStmt *node) = 0;
    virtual void visit(ExprStmt *node) = 0;
    virtual void visit(LetStmt *node) = 0;
    virtual void visit(ReturnStmt *node) = 0;
    virtual void visit(BreakStmt *node) = 0;
    virtual void visit(ContinueStmt *node) = 0;
    virtual void visit(ItemStmt *node) {}
};

class ItemVisitor {
  public:
    virtual ~ItemVisitor() = default;

    // Item visitors
    virtual void visit(FnDecl *node) = 0;
    virtual void visit(StructDecl *node) {}
    virtual void visit(ConstDecl *node) {}
    virtual void visit(EnumDecl *node) {}
    virtual void visit(EnumVariant *node) {}
    virtual void visit(ModDecl *node) {}
    virtual void visit(TraitDecl *node) {}
    virtual void visit(ImplBlock *node) {}
    virtual void visit(Program *node) = 0;
};

class TypeVisitor {
  public:
    virtual ~TypeVisitor() = default;

    // Type node visitors
    virtual void visit(TypeNameNode *node) = 0;
    virtual void visit(ArrayTypeNode *node) = 0;
    virtual void visit(UnitTypeNode *node) = 0;
    virtual void visit(TupleTypeNode *node) = 0;
    virtual void visit(PathTypeNode *node) {}
    virtual void visit(RawPointerTypeNode *node) {}
    virtual void visit(ReferenceTypeNode *node) {}
    virtual void visit(SliceTypeNode *node) {}
    virtual void visit(SelfTypeNode *node) {}
};

class PatternVisitor {
  public:
    virtual ~PatternVisitor() = default;

    // Pattern visitors
    virtual void visit(IdentifierPattern *node) = 0;
    virtual void visit(WildcardPattern *node) = 0;
    virtual void visit(LiteralPattern *node) = 0;
    virtual void visit(TuplePattern *node) = 0;
    virtual void visit(SlicePattern *node) = 0;
    virtual void visit(StructPattern *node) = 0;
    virtual void visit(RestPattern *node) = 0;
};

class OtherVisitor {
  public:
    virtual ~OtherVisitor() = default;

    // Other node visitors
    virtual void visit(FnParam *node){};
    virtual void visit(MatchArm *node){};
    virtual void visit(Field *node){};
    virtual void visit(FieldInitializer *node){};
    virtual void visit(StructPatternField *node){};
    virtual void visit(EnumVariant *node){};
};