// type_resolve.cpp

#include "semantic.h"

TypeResolver::TypeResolver(NameResolutionVisitor &resolver, SymbolTable &symbols,
                           ErrorReporter &reporter)
    : name_resolver_(resolver), symbol_table_(symbols), error_reporter_(reporter) {}

std::shared_ptr<Type> TypeResolver::resolve(TypeNode *node) {
    if (!node)
        return nullptr;
    if (node->resolved_type)
        return node->resolved_type;
    resolved_type_ = nullptr;
    node->accept(this);
    node->resolved_type = resolved_type_;

    return resolved_type_;
}

void TypeResolver::visit(TypeNameNode *node) {

    auto symbol = symbol_table_.lookup_type(node->name.lexeme);

    if (symbol && symbol->kind == Symbol::TYPE) {
        resolved_type_ = symbol->type;
        node->resolved_symbol = symbol;
    } else {
        error_reporter_.report_error("Unknown type name '" + node->name.lexeme + "'.",
                                     node->name.line);
        resolved_type_ = nullptr;
    }
}

void TypeResolver::visit(ArrayTypeNode *node) {

    auto element_type = resolve(node->element_type.get());

    if (!element_type) {
        resolved_type_ = nullptr;
        return;
    }

    if (node->size) {
        node->size->accept(&name_resolver_);
    }

    ConstEvaluator const_evaluator(symbol_table_, error_reporter_);
    auto size_opt = const_evaluator.evaluate(node->size.get());

    if (!size_opt) {
        error_reporter_.report_error("Array size must be a constant expression.");
        resolved_type_ = nullptr;
        return;
    }

    long long evaluated_size = *size_opt;
    if (evaluated_size < 0) {
        error_reporter_.report_error("Array size cannot be negative.");
        resolved_type_ = nullptr;
        return;
    }

    size_t size = static_cast<size_t>(evaluated_size);
    resolved_type_ = std::make_shared<ArrayType>(element_type, size);
}

void TypeResolver::visit(UnitTypeNode *node) { resolved_type_ = std::make_shared<UnitType>(); }

void TypeResolver::visit(TupleTypeNode *node) { resolved_type_ = std::make_shared<UnitType>(); }

void TypeResolver::visit(PathTypeNode *node) {

    if (auto *var_expr = dynamic_cast<VariableExpr *>(node->path.get())) {
        const auto &name = var_expr->name.lexeme;

        auto symbol = symbol_table_.lookup_type(var_expr->name.lexeme);

        if (symbol && symbol->kind == Symbol::TYPE) {

            resolved_type_ = symbol->type;
            node->resolved_symbol = symbol;
        } else {
            error_reporter_.report_error("Unknown type name '" + var_expr->name.lexeme + "'.");
            resolved_type_ = nullptr;
        }
    } else if (auto *path_expr = dynamic_cast<PathExpr *>(node->path.get())) {

        auto symbol = path_expr->accept(&name_resolver_);
        if (symbol && symbol->kind == Symbol::TYPE) {
            resolved_type_ = symbol->type;
            node->resolved_symbol = symbol;
        } else {
            resolved_type_ = nullptr;
        }
    } else {
        resolved_type_ = nullptr;
    }
}

void TypeResolver::visit(ReferenceTypeNode *node) {
    auto resolved_inner_type = resolve(node->referenced_type.get());

    if (!resolved_inner_type) {
        resolved_type_ = nullptr;
        return;
    }

    resolved_type_ = std::make_shared<ReferenceType>(resolved_inner_type, node->is_mutable);
}

void TypeResolver::visit(RawPointerTypeNode *node) {
    auto pointee_type = resolve(node->pointee_type.get());
    if (!pointee_type) {
        resolved_type_ = nullptr;
        return;
    }
    resolved_type_ = std::make_shared<RawPointerType>(pointee_type, node->is_mutable);
}

void TypeResolver::visit(SliceTypeNode *node) { resolved_type_ = nullptr; }

void TypeResolver::visit(SelfTypeNode *node) { resolved_type_ = nullptr; }
