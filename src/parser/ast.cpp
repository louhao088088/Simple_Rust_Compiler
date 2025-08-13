// ast.cpp
#include "ast.h"

#include <string>

// Helper function for printing indentation
static void print_indent(std::ostream &os, int indent) { os << std::string(indent * 2, ' '); }

// expr
void LiteralExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "LiteralExpr(" << literal.lexeme << ")\n";
}

void VariableExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "VariableExpr(" << name.lexeme << ")\n";
}

void UnaryExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "UnaryExpr(" << op.lexeme << ")\n";
    right->print(os, indent + 1);
}

void BinaryExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "BinaryExpr(" << op.lexeme << ")\n";
    left->print(os, indent + 1);
    right->print(os, indent + 1);
}

void CallExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "CallExpr\n";
    print_indent(os, indent + 1);
    os << "Callee:\n";
    callee->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Arguments:\n";
    for (const auto &arg : arguments) {
        arg->print(os, indent + 2);
    }
}

void IfExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "IfExpr\n";
    print_indent(os, indent + 1);
    os << "Condition:\n";
    condition->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Then:\n";
    then_branch->print(os, indent + 2);
    if (else_branch) {
        print_indent(os, indent + 1);
        os << "Else:\n";
        (*else_branch)->print(os, indent + 2);
    }
}

void IndexExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "IndexExpr\n";
    print_indent(os, indent + 1);
    os << "Object:\n";
    object->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Index:\n";
    index->print(os, indent + 2);
}

void FieldAccessExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "FieldAccessExpr(field=" << field.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Object:\n";
    object->print(os, indent + 2);
}

void ArrayLiteralExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ArrayLiteralExpr\n";
    print_indent(os, indent + 1);
    os << "Elements:\n";
    for (const auto &elem : elements) {
        elem->print(os, indent + 2);
    }
}

void ArrayInitializerExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ArrayInitializerExpr\n";
    print_indent(os, indent + 1);
    os << "Value:\n";
    value->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Size:\n";
    size->print(os, indent + 2);
}

void AssignmentExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "AssignmentExpr\n";

    print_indent(os, indent + 1);
    os << "Target:\n";
    target->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Value:\n";
    value->print(os, indent + 2);
}

void CompoundAssignmentExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "CompoundAssignmentExpr(" << op.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Target:\n";
    target->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Value:\n";
    value->print(os, indent + 2);
}

void LoopExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "LoopExpr\n";
    print_indent(os, indent + 1);
    os << "Body:\n";
    body->print(os, indent + 2);
}

void WhileExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "WhileExpr\n";
    print_indent(os, indent + 1);
    os << "Condition:\n";
    condition->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Body:\n";
    body->print(os, indent + 2);
}

void StructInitializerExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "StructInitializerExpr\n";
    print_indent(os, indent + 1);
    os << "Name:\n";
    name->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Fields:\n";
    for (const auto &field : fields) {
        print_indent(os, indent + 2);
        os << field.name.lexeme << ":\n";
        field.value->print(os, indent + 3);
    }
}

void UnitExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "UnitExpr\n";
}

void GroupingExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "GroupingExpr\n";
    print_indent(os, indent + 1);
    os << "Expression:\n";
    expression->print(os, indent + 2);
}

void TupleExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "TupleExpr\n";
    print_indent(os, indent + 1);
    os << "Elements:\n";
    for (const auto &elem : elements) {
        elem->print(os, indent + 2);
    }
}

void AsExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "AsExpr\n";
    print_indent(os, indent + 1);
    os << "Expression:\n";
    expression->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Target Type:\n";
    target_type->print(os, indent + 2);
}
void MatchArm::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "MatchArm\n";
    print_indent(os, indent + 1);
    os << "Pattern:\n";
    pattern->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Guard:\n";
    if (guard) {
        (*guard)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "Body:\n";
    body->print(os, indent + 2);
}

void MatchExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "MatchExpr\n";
    print_indent(os, indent + 1);
    os << "Subject:\n";
    scrutinee->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Arms:\n";
    for (const auto &arm : arms) {
        arm->print(os, indent + 2);
    }
}

void UnderscoreExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "UnderscoreExpr(_)\n";
}

void PathExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "PathExpr\n";
    print_indent(os, indent + 1);
    os << "Left:\n";
    left->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Operator: ";
    os << op.lexeme << "\n";
    print_indent(os, indent + 1);
    os << "Right: ";
    os << right.lexeme << "\n";
}

void ReferenceExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ReferenceExpr\n";
    print_indent(os, indent + 1);
    os << "Is Mutable: " << (is_mutable ? "true" : "false") << "\n";
    print_indent(os, indent + 1);
    os << "Expression:\n";
    expression->print(os, indent + 2);
}

// stmt
void BlockStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "BlockStmt\n";
    for (const auto &stmt : statements) {
        stmt->print(os, indent + 1);
    }
    if (final_expr) {
        print_indent(os, indent + 1);
        os << "Final Expression:\n";
        (*final_expr)->print(os, indent + 2);
    }
}

void ExprStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ExprStmt\n";
    expression->print(os, indent + 1);
}

void LetStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "LetStmt\n";
    print_indent(os, indent + 1);
    os << "Pattern:\n";
    pattern->print(os, indent + 2);
    if (type_annotation) {
        print_indent(os, indent + 1);
        os << "Type Annotation:\n";
        (*type_annotation)->print(os, indent + 2);
    }
    if (initializer) {
        print_indent(os, indent + 1);
        os << "Initializer:\n";
        (*initializer)->print(os, indent + 2);
    }
}

void ReturnStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ReturnStmt\n";
    if (value) {
        (*value)->print(os, indent + 1);
    }
}

void BreakStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "BreakStmt\n";
    if (value) {
        print_indent(os, indent + 1);
        os << "Value:\n";
        (*value)->print(os, indent + 2);
    }
}

void ContinueStmt::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ContinueStmt\n";
}

void ItemStmt::print(std::ostream &os, int indent) const { item->print(os, indent); }

void FnDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "FnDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Params: ";
    for (const auto &p : params) {
        os << p.lexeme << " ";
    }

    os << "\n";
    print_indent(os, indent + 1);
    os << "Param Types:\n";
    for (const auto &type : param_types) {
        type->print(os, indent + 1);
    }
    print_indent(os, indent + 1);
    os << "Return Type:\n";
    if (return_type) {
        (*return_type)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "Body:\n";
    body->print(os, indent + 2);
}

void StructDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "StructDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Kind: ";
    switch (kind) {
    case StructKind::Normal:
        os << "Normal\n";
        break;
    case StructKind::Tuple:
        os << "Tuple\n";
        break;
    case StructKind::Unit:
        os << "Unit\n";
        break;
    }
    print_indent(os, indent + 1);
    os << "Fields:\n";
    for (const auto &field : fields) {
        print_indent(os, indent + 2);
        os << "Field(name=" << field.name.lexeme << ")\n";
        print_indent(os, indent + 3);
        os << "Type:\n";
        field.type->print(os, indent + 4);
    }
}

void ConstDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ConstDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Type:\n";
    type->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Value:\n";
    value->print(os, indent + 2);
}

void EnumVariant::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "EnumVariant(name=" << name.lexeme;
    switch (kind) {
    case EnumVariantKind::Plain:
        os << ", kind=Plain)\n";
        if (discriminant) {
            print_indent(os, indent + 1);
            os << "Discriminant:\n";
            (*discriminant)->print(os, indent + 2);
        }
        break;
    case EnumVariantKind::Tuple:
        os << ", kind=Tuple)\n";
        print_indent(os, indent + 1);
        os << "Types:\n";
        for (const auto &type : tuple_types) {
            type->print(os, indent + 2);
        }
        break;
    case EnumVariantKind::Struct:
        os << ", kind=Struct)\n";
        print_indent(os, indent + 1);
        os << "Fields:\n";
        for (const auto &field : fields) {
            print_indent(os, indent + 2);
            os << "Field(name=" << field.name.lexeme << ")\n";
            print_indent(os, indent + 3);
            os << "Type:\n";
            field.type->print(os, indent + 4);
        }
        break;
    }
}

void EnumDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "EnumDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Variants:\n";
    for (const auto &variant : variants) {
        variant->print(os, indent + 2);
    }
}

void ModDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ModDecl(name=" << name.lexeme << ")\n";

    if (!items.empty()) {
        print_indent(os, indent + 1);
        os << "Items:\n";
        for (const auto &item : items) {
            item->print(os, indent + 2);
        }
    }
}

// Root
void Program::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "Program\n";
    for (const auto &item : items) {
        item->print(os, indent + 1);
    }
}

// Type
void TypeNameNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "TypeNameNode(" << name.lexeme << ")\n";
}

void ArrayTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ArrayTypeNode\n";
    print_indent(os, indent + 1);
    os << "Element Type:\n";
    element_type->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Size:\n";
    size->print(os, indent + 2);
}

void UnitTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "UnitTypeNode\n";
}

void TupleTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "TupleTypeNode\n";
    for (const auto &elem : elements) {
        elem->print(os, indent + 1);
    }
}

void PathTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "PathTypeNode\n";
    print_indent(os, indent + 1);
    os << "Path:\n";
    path->print(os, indent + 2);
}

void RawPointerTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "RawPointerTypeNode\n";
    print_indent(os, indent + 1);
    os << "Pointee Type:\n";
    pointee_type->print(os, indent + 2);
}

void ReferenceTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ReferenceTypeNode\n";
    print_indent(os, indent + 1);
    os << "Is Mutable: " << (is_mutable ? "true" : "false") << "\n";
    print_indent(os, indent + 1);
    os << "Referenced Type:\n";
    referenced_type->print(os, indent + 2);
}

// pattern

void IdentifierPattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "IdentifierPattern(name=" << name.lexeme << ")\n";
    print_indent(os, indent);
    os << "mutability:" << (is_mutable ? "mutable" : "immutable") << "\n";
}

void WildcardPattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "WildcardPattern\n";
}

void LiteralPattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "LiteralPattern(value=" << literal.lexeme << ")\n";
}

void TuplePattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "TuplePattern\n";
    print_indent(os, indent + 1);
    os << "Elements:\n";
    for (const auto &element : elements) {
        element->print(os, indent + 2);
    }
}
void print_struct_pattern_field(const StructPatternField &field, std::ostream &os, int indent) {
    print_indent(os, indent);
    os << "StructPatternField(name=" << field.field_name.lexeme << ")\n";
    if (field.pattern) {
        print_indent(os, indent + 1);
        os << "Pattern:\n";
        (*field.pattern)->print(os, indent + 2);
    } else {
        print_indent(os, indent + 1);
        os << "Pattern: (shorthand)\n";
    }
}
void StructPattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "StructPatternNode(has_rest=" << (has_rest ? "true" : "false") << ")\n";

    print_indent(os, indent + 1);
    os << "Path:\n";
    path->print(os, indent + 2);

    print_indent(os, indent + 1);
    os << "Fields:\n";
    for (const auto &field : fields) {
        print_struct_pattern_field(field, os, indent + 2);
    }
}