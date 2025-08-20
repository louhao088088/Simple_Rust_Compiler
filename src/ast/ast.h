// ast.h
#pragma once

#include "../lexer/lexer.h"
#include "visit.h"

// Base classes with specialized accept methods

struct Node {
    virtual ~Node() = default;
    virtual void print(std::ostream &os, int indent = 0) const = 0;
};

struct Expr : public Node {
    // Semantic analysis annotations
    std::shared_ptr<Type> type;
    std::shared_ptr<Symbol> resolved_symbol;
    virtual std::shared_ptr<Symbol> accept(ExprVisitor *visitor) = 0;
};

struct Stmt : public Node {
    virtual void accept(StmtVisitor *visitor) = 0;
};

struct Item : public Node {
    virtual void accept(ItemVisitor *visitor) = 0;
};

struct TypeNode : public Node {
    virtual void accept(TypeVisitor *visitor) = 0;
};

struct Pattern : public Node {
    virtual void accept(PatternVisitor *visitor) = 0;
};

// Expressions

struct LiteralExpr : public Expr {
    Token literal;
    explicit LiteralExpr(Token lit) : literal(std::move(lit)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct ArrayLiteralExpr : public Expr {
    std::vector<std::shared_ptr<Expr>> elements;

    ArrayLiteralExpr(std::vector<std::shared_ptr<Expr>> elems) : elements(std::move(elems)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct ArrayInitializerExpr : public Expr {
    std::shared_ptr<Expr> value;
    std::shared_ptr<Expr> size;

    ArrayInitializerExpr(std::shared_ptr<Expr> val, std::shared_ptr<Expr> cnt)
        : value(std::move(val)), size(std::move(cnt)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct VariableExpr : public Expr {
    Token name;
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct UnaryExpr : public Expr {
    Token op;
    std::shared_ptr<Expr> right;
    UnaryExpr(Token op, std::shared_ptr<Expr> right) : op(std::move(op)), right(std::move(right)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct BinaryExpr : public Expr {
    std::shared_ptr<Expr> left;
    Token op;
    std::shared_ptr<Expr> right;
    BinaryExpr(std::shared_ptr<Expr> left, Token op, std::shared_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct CallExpr : public Expr {
    std::shared_ptr<Expr> callee;
    std::vector<std::shared_ptr<Expr>> arguments;
    CallExpr(std::shared_ptr<Expr> callee, std::vector<std::shared_ptr<Expr>> args)
        : callee(std::move(callee)), arguments(std::move(args)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct IfExpr : public Expr {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> then_branch;
    std::optional<std::shared_ptr<Stmt>> else_branch;
    IfExpr(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> then_b,
           std::optional<std::shared_ptr<Stmt>> else_b)
        : condition(std::move(cond)), then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct LoopExpr : public Expr {
    std::shared_ptr<Stmt> body;
    LoopExpr(std::shared_ptr<Stmt> body) : body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct WhileExpr : public Expr {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
    WhileExpr(std::shared_ptr<Expr> cond, std::shared_ptr<Stmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct IndexExpr : public Expr {
    std::shared_ptr<Expr> object;
    std::shared_ptr<Expr> index;

    IndexExpr(std::shared_ptr<Expr> obj, std::shared_ptr<Expr> idx)
        : object(std::move(obj)), index(std::move(idx)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct FieldAccessExpr : public Expr {
    std::shared_ptr<Expr> object;
    Token field;

    FieldAccessExpr(std::shared_ptr<Expr> obj, Token fld)
        : object(std::move(obj)), field(std::move(fld)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct AssignmentExpr : public Expr {
    std::shared_ptr<Expr> target;
    std::shared_ptr<Expr> value;
    AssignmentExpr(std::shared_ptr<Expr> t, std::shared_ptr<Expr> v)
        : target(std::move(t)), value(std::move(v)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct CompoundAssignmentExpr : public Expr {
    std::shared_ptr<Expr> target;
    Token op;
    std::shared_ptr<Expr> value;
    CompoundAssignmentExpr(std::shared_ptr<Expr> t, Token o, std::shared_ptr<Expr> v)
        : target(std::move(t)), op(std::move(o)), value(std::move(v)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct FieldInitializer : public Node {
    Token name;
    std::shared_ptr<Expr> value;

    FieldInitializer(Token n, std::shared_ptr<Expr> v) : name(std::move(n)), value(std::move(v)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

struct StructInitializerExpr : public Expr {
    std::shared_ptr<Expr> name;
    std::vector<std::shared_ptr<FieldInitializer>> fields;
    StructInitializerExpr(std::shared_ptr<Expr> n, std::vector<std::shared_ptr<FieldInitializer>> f)
        : name(std::move(n)), fields(std::move(f)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct UnitExpr : public Expr {
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct GroupingExpr : public Expr {
    std::shared_ptr<Expr> expression;
    explicit GroupingExpr(std::shared_ptr<Expr> expr) : expression(std::move(expr)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct TupleExpr : public Expr {
    std::vector<std::shared_ptr<Expr>> elements;
    explicit TupleExpr(std::vector<std::shared_ptr<Expr>> elems) : elements(std::move(elems)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct AsExpr : public Expr {
    std::shared_ptr<Expr> expression;
    std::shared_ptr<TypeNode> target_type;
    AsExpr(std::shared_ptr<Expr> expr, std::shared_ptr<TypeNode> type)
        : expression(std::move(expr)), target_type(std::move(type)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct MatchArm;
struct MatchExpr : public Expr {
    std::shared_ptr<Expr> scrutinee;
    std::vector<std::shared_ptr<MatchArm>> arms;

    MatchExpr(std::shared_ptr<Expr> scrut, std::vector<std::shared_ptr<MatchArm>> arms_vec)
        : scrutinee(std::move(scrut)), arms(std::move(arms_vec)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct UnderscoreExpr : public Expr {
    Token underscore_token;
    explicit UnderscoreExpr(Token token) : underscore_token(std::move(token)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct PathExpr : public Expr {
    std::shared_ptr<Expr> left;
    Token op;
    std::shared_ptr<Expr> right;

    PathExpr(std::shared_ptr<Expr> l, Token o, std::shared_ptr<Expr> r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct ReferenceExpr : public Expr {
    bool is_mutable;
    std::shared_ptr<Expr> expression;

    ReferenceExpr(bool is_mut, std::shared_ptr<Expr> expr)
        : is_mutable(is_mut), expression(std::move(expr)) {}

    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

struct BlockExpr : public Expr {
    std::shared_ptr<BlockStmt> block_stmt;
    explicit BlockExpr(std::shared_ptr<BlockStmt> stmt) : block_stmt(std::move(stmt)) {}
    void print(std::ostream &os, int indent = 0) const override;
    std::shared_ptr<Symbol> accept(ExprVisitor *visitor) override;
};

// Statements

struct BlockStmt : public Stmt {
    std::vector<std::shared_ptr<Stmt>> statements;
    std::optional<std::shared_ptr<Expr>> final_expr;
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct ExprStmt : public Stmt {
    std::shared_ptr<Expr> expression;
    bool has_semicolon;
    explicit ExprStmt(std::shared_ptr<Expr> expr, bool has_semi = false)
        : expression(std::move(expr)), has_semicolon(has_semi) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct LetStmt : public Stmt {
    std::shared_ptr<Pattern> pattern;
    std::optional<std::shared_ptr<TypeNode>> type_annotation;
    std::optional<std::shared_ptr<Expr>> initializer;
    LetStmt(std::shared_ptr<Pattern> pat, std::optional<std::shared_ptr<TypeNode>> type_ann,
            std::optional<std::shared_ptr<Expr>> init)
        : pattern(std::move(pat)), type_annotation(std::move(type_ann)),
          initializer(std::move(init)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct ReturnStmt : public Stmt {
    Token keyword;
    std::optional<std::shared_ptr<Expr>> value;
    ReturnStmt(Token keyword, std::optional<std::shared_ptr<Expr>> val)
        : keyword(std::move(keyword)), value(std::move(val)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct BreakStmt : public Stmt {
    std::optional<std::shared_ptr<Expr>> value;
    BreakStmt(std::optional<std::shared_ptr<Expr>> value) : value(std::move(value)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct ContinueStmt : public Stmt {
    ContinueStmt() = default;
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

struct ItemStmt : public Stmt {
    std::shared_ptr<Item> item;
    explicit ItemStmt(std::shared_ptr<Item> i) : item(std::move(i)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(StmtVisitor *visitor) override;
};

// Type Nodes
struct TypeNameNode : public TypeNode {
    Token name;
    explicit TypeNameNode(Token name) : name(std::move(name)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct ArrayTypeNode : public TypeNode {
    std::shared_ptr<TypeNode> element_type;
    std::shared_ptr<Expr> size;
    ArrayTypeNode(std::shared_ptr<TypeNode> et, std::shared_ptr<Expr> sz)
        : element_type(std::move(et)), size(std::move(sz)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct UnitTypeNode : public TypeNode {
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct TupleTypeNode : public TypeNode {
    std::vector<std::shared_ptr<TypeNode>> elements;
    explicit TupleTypeNode(std::vector<std::shared_ptr<TypeNode>> elems)
        : elements(std::move(elems)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct PathTypeNode : public TypeNode {
    std::shared_ptr<Expr> path;
    std::optional<std::vector<std::shared_ptr<TypeNode>>> generic_args;
    PathTypeNode(std::shared_ptr<Expr> p,
                 std::optional<std::vector<std::shared_ptr<TypeNode>>> args = std::nullopt)
        : path(std::move(p)), generic_args(std::move(args)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct RawPointerTypeNode : public TypeNode {
    bool is_mutable;
    std::shared_ptr<TypeNode> pointee_type;
    RawPointerTypeNode(bool is_mut, std::shared_ptr<TypeNode> pointee)
        : is_mutable(is_mut), pointee_type(std::move(pointee)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct ReferenceTypeNode : public TypeNode {
    bool is_mutable;
    std::shared_ptr<TypeNode> referenced_type;
    ReferenceTypeNode(bool is_mut, std::shared_ptr<TypeNode> ref_type)
        : is_mutable(is_mut), referenced_type(std::move(ref_type)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct SliceTypeNode : public TypeNode {
    std::shared_ptr<TypeNode> element_type;
    SliceTypeNode(std::shared_ptr<TypeNode> elem_type) : element_type(std::move(elem_type)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

struct SelfTypeNode : public TypeNode {
    void print(std::ostream &os, int indent = 0) const override;
    void accept(TypeVisitor *visitor) override;
};

// Top-level Items
struct FnParam : public Node {
    std::shared_ptr<Pattern> pattern;
    std::shared_ptr<TypeNode> type;
    FnParam(std::shared_ptr<Pattern> p, std::shared_ptr<TypeNode> t)
        : pattern(std::move(p)), type(std::move(t)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

struct FnDecl : public Item {
    Token name;
    std::vector<std::shared_ptr<FnParam>> params;
    std::optional<std::shared_ptr<TypeNode>> return_type;
    std::optional<std::shared_ptr<BlockStmt>> body;

    FnDecl(Token name, std::vector<std::shared_ptr<FnParam>> params,
           std::optional<std::shared_ptr<TypeNode>> return_type,
           std::optional<std::shared_ptr<BlockStmt>> body)
        : name(std::move(name)), params(std::move(params)), return_type(std::move(return_type)),
          body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;

    std::shared_ptr<Symbol> resolved_symbol;
};

struct Field : public Node {
    Token name;
    std::shared_ptr<TypeNode> type;
    Field(Token n, std::shared_ptr<TypeNode> t) : name(std::move(n)), type(std::move(t)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

enum class StructKind { Normal, Tuple, Unit };

struct StructDecl : public Item {
    Token name;
    StructKind kind;
    std::vector<std::shared_ptr<Field>> fields;
    std::vector<std::shared_ptr<TypeNode>> tuple_fields;

    StructDecl(Token n, std::vector<std::shared_ptr<Field>> f) // Normal
        : name(std::move(n)), kind(StructKind::Normal), fields(std::move(f)) {}

    StructDecl(Token n, std::vector<std::shared_ptr<TypeNode>> tf) // Tuple
        : name(std::move(n)), kind(StructKind::Tuple), tuple_fields(std::move(tf)) {}

    explicit StructDecl(Token n) // Unit
        : name(std::move(n)), kind(StructKind::Unit) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

struct ConstDecl : public Item {
    Token name;
    std::shared_ptr<TypeNode> type;
    std::shared_ptr<Expr> value;

    ConstDecl(Token n, std::shared_ptr<TypeNode> t, std::shared_ptr<Expr> v)
        : name(std::move(n)), type(std::move(t)), value(std::move(v)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

enum class EnumVariantKind { Plain, Tuple, Struct };

struct EnumVariant : public Node {
    Token name;
    EnumVariantKind kind;

    std::optional<std::shared_ptr<Expr>> discriminant;
    std::vector<std::shared_ptr<TypeNode>> tuple_types;
    std::vector<std::shared_ptr<Field>> fields;

    EnumVariant(Token n, std::optional<std::shared_ptr<Expr>> disc = std::nullopt)
        : name(std::move(n)), kind(EnumVariantKind::Plain), discriminant(std::move(disc)) {}

    EnumVariant(Token n, std::vector<std::shared_ptr<TypeNode>> types)
        : name(std::move(n)), kind(EnumVariantKind::Tuple), tuple_types(std::move(types)) {}

    EnumVariant(Token n, std::vector<std::shared_ptr<Field>> f)
        : name(std::move(n)), kind(EnumVariantKind::Struct), fields(std::move(f)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

struct EnumDecl : public Item {
    Token name;
    std::vector<std::shared_ptr<EnumVariant>> variants;

    EnumDecl(Token n, std::vector<std::shared_ptr<EnumVariant>> v)
        : name(std::move(n)), variants(std::move(v)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

struct ModDecl : public Item {
    Token name;
    std::vector<std::shared_ptr<Item>> items;

    ModDecl(Token n, std::vector<std::shared_ptr<Item>> i)
        : name(std::move(n)), items(std::move(i)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

struct TraitDecl : public Item {
    Token name;
    std::vector<std::shared_ptr<Item>> associated_items;
    TraitDecl(Token n, std::vector<std::shared_ptr<Item>> items)
        : name(std::move(n)), associated_items(std::move(items)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

struct ImplBlock : public Item {
    std::optional<std::shared_ptr<TypeNode>> trait_name;
    std::shared_ptr<TypeNode> target_type;
    std::vector<std::shared_ptr<Item>> implemented_items;

    ImplBlock(std::optional<std::shared_ptr<TypeNode>> trait, std::shared_ptr<TypeNode> target,
              std::vector<std::shared_ptr<Item>> items)
        : trait_name(std::move(trait)), target_type(std::move(target)),
          implemented_items(std::move(items)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};

// pattern node
struct WildcardPattern : public Pattern {
    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct LiteralPattern : public Pattern {
    Token literal;
    explicit LiteralPattern(Token lit) : literal(std::move(lit)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct IdentifierPattern : public Pattern {
    Token name;
    bool is_mutable;
    IdentifierPattern(Token n, bool is_mut) : name(std::move(n)), is_mutable(is_mut) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct TuplePattern : public Pattern {
    std::vector<std::shared_ptr<Pattern>> elements;

    explicit TuplePattern(std::vector<std::shared_ptr<Pattern>> elems)
        : elements(std::move(elems)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct MatchArm : public Node {
    std::shared_ptr<Pattern> pattern;
    std::optional<std::shared_ptr<Expr>> guard;
    std::shared_ptr<Expr> body;

    MatchArm(std::shared_ptr<Pattern> pat, std::optional<std::shared_ptr<Expr>> grd,
             std::shared_ptr<Expr> bdy)
        : pattern(std::move(pat)), guard(std::move(grd)), body(std::move(bdy)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

struct StructPatternField : public Node {
    Token field_name;
    std::optional<std::shared_ptr<Pattern>> pattern;
    StructPatternField(Token name, std::optional<std::shared_ptr<Pattern>> pat = std::nullopt)
        : field_name(std::move(name)), pattern(std::move(pat)) {}
    void print(std::ostream &os, int indent = 0) const override;
    void accept(OtherVisitor *visitor);
};

struct StructPattern : public Pattern {
    std::shared_ptr<Expr> path;
    std::vector<std::shared_ptr<StructPatternField>> fields;
    bool has_rest;

    StructPattern(std::shared_ptr<Expr> p, std::vector<std::shared_ptr<StructPatternField>> f,
                  bool rest)
        : path(std::move(p)), fields(std::move(f)), has_rest(rest) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct RestPattern : public Pattern {
    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

struct SlicePattern : public Pattern {
    std::vector<std::shared_ptr<Pattern>> elements;
    explicit SlicePattern(std::vector<std::shared_ptr<Pattern>> elems)
        : elements(std::move(elems)) {}

    void print(std::ostream &os, int indent = 0) const override;
    void accept(PatternVisitor *visitor) override;
};

// Root node
struct Program : public Item {
    std::vector<std::shared_ptr<Item>> items;
    void print(std::ostream &os, int indent = 0) const override;
    void accept(ItemVisitor *visitor) override;
};
