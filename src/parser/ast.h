// ast.h
#pragma once

#include "../lexer/lexer.h"

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declarations of all nodes
struct Node;
struct Expr;
struct Stmt;
struct Item;
struct Program;

// Base classes

struct Node {
    virtual ~Node() = default;
    virtual void print(std::ostream &os, int indent = 0) const = 0;
};

struct Expr : public Node {};
struct Stmt : public Node {};
struct Item : public Node {};
struct TypeNode : public Node {};
struct Pattern : public Node {};

// Expressions

struct LiteralExpr : public Expr {
    Token literal;
    explicit LiteralExpr(Token lit) : literal(std::move(lit)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct ArrayLiteralExpr : public Expr {
    std::vector<std::unique_ptr<Expr>> elements;

    ArrayLiteralExpr(std::vector<std::unique_ptr<Expr>> elems) : elements(std::move(elems)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct ArrayInitializerExpr : public Expr {
    std::unique_ptr<Expr> value;
    std::unique_ptr<Expr> size;

    ArrayInitializerExpr(std::unique_ptr<Expr> val, std::unique_ptr<Expr> cnt)
        : value(std::move(val)), size(std::move(cnt)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct VariableExpr : public Expr {
    Token name;
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct UnaryExpr : public Expr {
    Token op;
    std::unique_ptr<Expr> right;
    UnaryExpr(Token op, std::unique_ptr<Expr> right) : op(std::move(op)), right(std::move(right)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct BinaryExpr : public Expr {
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct CallExpr : public Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::unique_ptr<Expr> callee, std::vector<std::unique_ptr<Expr>> args)
        : callee(std::move(callee)), arguments(std::move(args)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct IfExpr : public Expr {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::optional<std::unique_ptr<Stmt>> else_branch;
    IfExpr(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_b,
           std::optional<std::unique_ptr<Stmt>> else_b)
        : condition(std::move(cond)), then_branch(std::move(then_b)),
          else_branch(std::move(else_b)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct LoopExpr : public Expr {
    std::unique_ptr<Stmt> body;
    LoopExpr(std::unique_ptr<Stmt> body) : body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct WhileExpr : public Expr {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileExpr(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
        : condition(std::move(cond)), body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct IndexExpr : public Expr {
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;

    IndexExpr(std::unique_ptr<Expr> obj, std::unique_ptr<Expr> idx)
        : object(std::move(obj)), index(std::move(idx)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct FieldAccessExpr : public Expr {
    std::unique_ptr<Expr> object;
    Token field;

    FieldAccessExpr(std::unique_ptr<Expr> obj, Token fld)
        : object(std::move(obj)), field(std::move(fld)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct AssignmentExpr : public Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
    AssignmentExpr(std::unique_ptr<Expr> t, std::unique_ptr<Expr> v)
        : target(std::move(t)), value(std::move(v)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct CompoundAssignmentExpr : public Expr {
    std::unique_ptr<Expr> target;
    Token op;
    std::unique_ptr<Expr> value;
    CompoundAssignmentExpr(std::unique_ptr<Expr> t, Token o, std::unique_ptr<Expr> v)
        : target(std::move(t)), op(std::move(o)), value(std::move(v)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct FieldInitializer {
    Token name;
    std::unique_ptr<Expr> value;
};

struct StructInitializerExpr : public Expr {
    std::unique_ptr<Expr> name;
    std::vector<FieldInitializer> fields;
    StructInitializerExpr(std::unique_ptr<Expr> n, std::vector<FieldInitializer> f)
        : name(std::move(n)), fields(std::move(f)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct UnitExpr : public Expr {
    void print(std::ostream &os, int indent = 0) const override;
};

struct GroupingExpr : public Expr {
    std::unique_ptr<Expr> expression;
    explicit GroupingExpr(std::unique_ptr<Expr> expr) : expression(std::move(expr)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct TupleExpr : public Expr {
    std::vector<std::unique_ptr<Expr>> elements;
    explicit TupleExpr(std::vector<std::unique_ptr<Expr>> elems) : elements(std::move(elems)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct AsExpr : public Expr {
    std::unique_ptr<Expr> expression;
    std::unique_ptr<TypeNode> target_type;
    AsExpr(std::unique_ptr<Expr> expr, std::unique_ptr<TypeNode> type)
        : expression(std::move(expr)), target_type(std::move(type)) {}
    void print(std::ostream &os, int indent = 0) const override;
};
struct MatchArm;
struct MatchExpr : public Expr {
    std::unique_ptr<Expr> scrutinee; // The expression being matched on
    std::vector<std::unique_ptr<MatchArm>> arms;

    MatchExpr(std::unique_ptr<Expr> scrut, std::vector<std::unique_ptr<MatchArm>> arms_vec)
        : scrutinee(std::move(scrut)), arms(std::move(arms_vec)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct UnderscoreExpr : public Expr {
    Token underscore_token;
    explicit UnderscoreExpr(Token token) : underscore_token(std::move(token)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct PathExpr : public Expr {
    std::unique_ptr<Expr> left;
    Token op;
    Token right;

    PathExpr(std::unique_ptr<Expr> l, Token o, Token r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct ReferenceExpr : public Expr {
    bool is_mutable;
    std::unique_ptr<Expr> expression;

    ReferenceExpr(bool is_mut, std::unique_ptr<Expr> expr)
        : is_mutable(is_mut), expression(std::move(expr)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

// Statements

struct BlockStmt : public Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    std::optional<std::unique_ptr<Expr>> final_expr;
    void print(std::ostream &os, int indent = 0) const override;
};

struct ExprStmt : public Stmt {
    std::unique_ptr<Expr> expression;
    bool has_semicolon;
    ExprStmt(std::unique_ptr<Expr> expr, bool semi)
        : expression(std::move(expr)), has_semicolon(semi) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct LetStmt : public Stmt {

    std::unique_ptr<Pattern> pattern;

    std::optional<std::unique_ptr<TypeNode>> type_annotation;
    std::optional<std::unique_ptr<Expr>> initializer;

    LetStmt(std::unique_ptr<Pattern> pat, std::optional<std::unique_ptr<TypeNode>> type_ann,
            std::optional<std::unique_ptr<Expr>> init)
        : pattern(std::move(pat)), type_annotation(std::move(type_ann)),
          initializer(std::move(init)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct ReturnStmt : public Stmt {
    Token keyword;
    std::optional<std::unique_ptr<Expr>> value;
    ReturnStmt(Token keyword, std::optional<std::unique_ptr<Expr>> val)
        : keyword(std::move(keyword)), value(std::move(val)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct BreakStmt : public Stmt {
    std::optional<std::unique_ptr<Expr>> value;
    BreakStmt(std::optional<std::unique_ptr<Expr>> value) : value(std::move(value)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct ContinueStmt : public Stmt {
    ContinueStmt() = default;
    void print(std::ostream &os, int indent = 0) const override;
};

struct ItemStmt : public Stmt {
    std::unique_ptr<Item> item;
    explicit ItemStmt(std::unique_ptr<Item> i) : item(std::move(i)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

// Type Nodes
struct TypeNameNode : public TypeNode {
    Token name;
    explicit TypeNameNode(Token name) : name(std::move(name)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct ArrayTypeNode : public TypeNode {
    std::unique_ptr<TypeNode> element_type;
    std::unique_ptr<Expr> size;
    ArrayTypeNode(std::unique_ptr<TypeNode> et, std::unique_ptr<Expr> sz)
        : element_type(std::move(et)), size(std::move(sz)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct UnitTypeNode : public TypeNode {
    void print(std::ostream &os, int indent = 0) const override;
};

struct TupleTypeNode : public TypeNode {
    std::vector<std::unique_ptr<TypeNode>> elements;
    explicit TupleTypeNode(std::vector<std::unique_ptr<TypeNode>> elems)
        : elements(std::move(elems)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct PathTypeNode : public TypeNode {
    std::unique_ptr<Expr> path;
    explicit PathTypeNode(std::unique_ptr<Expr> p) : path(std::move(p)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct RawPointerTypeNode : public TypeNode {
    bool is_mutable;
    std::unique_ptr<TypeNode> pointee_type;
    RawPointerTypeNode(bool is_mut, std::unique_ptr<TypeNode> pointee)
        : is_mutable(is_mut), pointee_type(std::move(pointee)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct ReferenceTypeNode : public TypeNode {
    bool is_mutable;
    std::unique_ptr<TypeNode> referenced_type;
    ReferenceTypeNode(bool is_mut, std::unique_ptr<TypeNode> ref_type)
        : is_mutable(is_mut), referenced_type(std::move(ref_type)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

// Top-level Items

struct FnDecl : public Item {
    Token name;
    std::vector<Token> params;
    std::vector<std::unique_ptr<TypeNode>> param_types;
    std::optional<std::unique_ptr<TypeNode>> return_type;
    std::unique_ptr<BlockStmt> body;

    FnDecl(Token name, std::vector<Token> params,
           std::vector<std::unique_ptr<TypeNode>> param_types,
           std::optional<std::unique_ptr<TypeNode>> return_type, std::unique_ptr<BlockStmt> body)
        : name(std::move(name)), params(std::move(params)), param_types(std::move(param_types)),
          return_type(std::move(return_type)), body(std::move(body)) {}
    void print(std::ostream &os, int indent = 0) const override;
};
struct Field {
    Token name;
    std::unique_ptr<TypeNode> type;
};
enum class StructKind { Normal, Tuple, Unit };

struct StructDecl : public Item {
    Token name;
    StructKind kind;
    std::vector<Field> fields;
    std::vector<std::unique_ptr<TypeNode>> tuple_fields;

    StructDecl(Token n, std::vector<Field> f) // Normal
        : name(std::move(n)), kind(StructKind::Normal), fields(std::move(f)) {}

    StructDecl(Token n, std::vector<std::unique_ptr<TypeNode>> tf) // Tuple
        : name(std::move(n)), kind(StructKind::Tuple), tuple_fields(std::move(tf)) {}

    explicit StructDecl(Token n) // Unit
        : name(std::move(n)), kind(StructKind::Unit) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct ConstDecl : public Item {
    Token name;
    std::unique_ptr<TypeNode> type;
    std::unique_ptr<Expr> value;

    ConstDecl(Token n, std::unique_ptr<TypeNode> t, std::unique_ptr<Expr> v)
        : name(std::move(n)), type(std::move(t)), value(std::move(v)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

enum class EnumVariantKind { Plain, Tuple, Struct };

struct EnumVariant : public Node {
    Token name;
    EnumVariantKind kind;

    std::optional<std::unique_ptr<Expr>> discriminant;

    std::vector<std::unique_ptr<TypeNode>> tuple_types;

    std::vector<Field> fields;

    EnumVariant(Token n, std::optional<std::unique_ptr<Expr>> disc = std::nullopt)
        : name(std::move(n)), kind(EnumVariantKind::Plain), discriminant(std::move(disc)) {}

    EnumVariant(Token n, std::vector<std::unique_ptr<TypeNode>> types)
        : name(std::move(n)), kind(EnumVariantKind::Tuple), tuple_types(std::move(types)) {}

    EnumVariant(Token n, std::vector<Field> f)
        : name(std::move(n)), kind(EnumVariantKind::Struct), fields(std::move(f)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct EnumDecl : public Item {
    Token name;
    std::vector<std::unique_ptr<EnumVariant>> variants;

    EnumDecl(Token n, std::vector<std::unique_ptr<EnumVariant>> v)
        : name(std::move(n)), variants(std::move(v)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct ModDecl : public Item {
    Token name;

    std::vector<std::unique_ptr<Item>> items;

    ModDecl(Token n, std::vector<std::unique_ptr<Item>> i)
        : name(std::move(n)), items(std::move(i)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

// pattern node
struct WildcardPattern : public Pattern {
    void print(std::ostream &os, int indent = 0) const override;
};

struct LiteralPattern : public Pattern {
    Token literal;
    explicit LiteralPattern(Token lit) : literal(std::move(lit)) {}
    void print(std::ostream &os, int indent = 0) const override;
};

struct IdentifierPattern : public Pattern {
    Token name;
    bool is_mutable;
    IdentifierPattern(Token n, bool is_mut) : name(std::move(n)), is_mutable(is_mut) {}
    void print(std::ostream &os, int indent = 0) const override;
};
struct TuplePattern : public Pattern {
    std::vector<std::unique_ptr<Pattern>> elements;

    explicit TuplePattern(std::vector<std::unique_ptr<Pattern>> elems)
        : elements(std::move(elems)) {}

    void print(std::ostream &os, int indent = 0) const override;
};
struct MatchArm : public Node {
    std::unique_ptr<Pattern> pattern;
    std::optional<std::unique_ptr<Expr>> guard;
    std::unique_ptr<Expr> body;

    MatchArm(std::unique_ptr<Pattern> pat, std::optional<std::unique_ptr<Expr>> grd,
             std::unique_ptr<Expr> bdy)
        : pattern(std::move(pat)), guard(std::move(grd)), body(std::move(bdy)) {}

    void print(std::ostream &os, int indent = 0) const override;
};

struct StructPatternField {
    Token field_name;
    std::optional<std::unique_ptr<Pattern>> pattern;
};

struct StructPattern : public Pattern {
    std::unique_ptr<Expr> path;

    std::vector<StructPatternField> fields;
    bool has_rest;

    StructPattern(std::unique_ptr<Expr> p, std::vector<StructPatternField> f, bool rest)
        : path(std::move(p)), fields(std::move(f)), has_rest(rest) {}

    void print(std::ostream &os, int indent = 0) const override;
};

// Root node

struct Program : public Node {
    std::vector<std::unique_ptr<Item>> items;
    void print(std::ostream &os, int indent = 0) const override;
};