#include "semantic.h"
// TypeResolver Implementation

TypeResolver::TypeResolver(NameResolutionVisitor &resolver, SymbolTable &symbols,
                           ErrorReporter &reporter)
    : name_resolver_(resolver), symbol_table_(symbols), error_reporter_(reporter) {}

std::shared_ptr<Type> TypeResolver::resolve(TypeNode *node) {
    if (!node)
        return nullptr;

    resolved_type_ = nullptr;
    node->accept(this);
    return resolved_type_;
}

void TypeResolver::visit(TypeNameNode *node) {
    // Handle primitive types
    if (node->name.lexeme == "i32") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::I32);
    } else if (node->name.lexeme == "u32") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::U32);
    } else if (node->name.lexeme == "isize") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::ISIZE);
    } else if (node->name.lexeme == "usize") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::USIZE);
    } else if (node->name.lexeme == "bool") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::BOOL);
    } else if (node->name.lexeme == "char") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::CHAR);
    } else if (node->name.lexeme == "string") {
        resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::STRING);
    } else {
        auto symbol = symbol_table_.lookup(node->name.lexeme);
        if (symbol && symbol->kind == Symbol::TYPE) {
            resolved_type_ = symbol->type;
            node->resolved_symbol = symbol;
        } else {
            resolved_type_ = nullptr;
        }
    }
}

void TypeResolver::visit(ArrayTypeNode *node) {
    auto element_type = resolve(node->element_type.get());
    if (!element_type) {
        resolved_type_ = nullptr;
        return;
    }

    ConstEvaluator const_evaluator(symbol_table_, error_reporter_);
    auto size_opt = const_evaluator.evaluate(node->size.get());

    if (!size_opt) {
        error_reporter_.report_error("Array size must be a constant expression.");
        resolved_type_ = nullptr;
        return;
    }

    size_t size = *size_opt;
    resolved_type_ = std::make_shared<ArrayType>(element_type, size);
}

void TypeResolver::visit(UnitTypeNode *node) { resolved_type_ = std::make_shared<UnitType>(); }

void TypeResolver::visit(TupleTypeNode *node) {
    // For now, just create a unit type - full tuple support later
    resolved_type_ = std::make_shared<UnitType>();
}

void TypeResolver::visit(PathTypeNode *node) {

    if (auto *var_expr = dynamic_cast<VariableExpr *>(node->path.get())) {
        const auto &name = var_expr->name.lexeme;

        if (name == "i32") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::I32);
        } else if (name == "u32") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::U32);
        } else if (name == "isize") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::ISIZE);
        } else if (name == "usize") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::USIZE);
        } else if (name == "anyint") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::ANY_INTEGER);
        } else if (name == "bool") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::BOOL);
        } else if (name == "char") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::CHAR);
        } else if (name == "string") {
            resolved_type_ = std::make_shared<PrimitiveType>(TypeKind::STRING);
        } else {
            auto symbol = symbol_table_.lookup(name);
            if (symbol && symbol->kind == Symbol::TYPE) {
                resolved_type_ = symbol->type;
                node->resolved_symbol = symbol;
            } else {
                resolved_type_ = nullptr;
            }
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

void TypeResolver::visit(RawPointerTypeNode *node) {
    // Placeholder for pointer types
    resolved_type_ = nullptr;
}

void TypeResolver::visit(ReferenceTypeNode *node) {
    auto resolved_inner_type = resolve(node->referenced_type.get());

    if (!resolved_inner_type) {
        resolved_type_ = nullptr;
        return;
    }

    resolved_type_ = std::make_shared<ReferenceType>(
        resolved_inner_type,
        node->is_mutable
    );
}

void TypeResolver::visit(SliceTypeNode *node) { resolved_type_ = nullptr; }

void TypeResolver::visit(SelfTypeNode *node) { resolved_type_ = nullptr; }