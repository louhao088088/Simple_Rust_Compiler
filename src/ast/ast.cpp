// ast.cpp
#include "ast.h"

#include <string>

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
        os << field->name.lexeme << ":\n";
        field->value->print(os, indent + 3);
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
    right->print(os, indent + 2);
}

void ReferenceExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ReferenceExpr\n";
    print_indent(os, indent + 1);
    os << "Mutiability: " << (is_mutable ? "true" : "false") << "\n";
    print_indent(os, indent + 1);
    os << "Expression:\n";
    expression->print(os, indent + 2);
}
void BlockExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "BlockExpr\n";
    block_stmt->print(os, indent + 1);
}

void SelfTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "SelfTypeNode\n";
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
    os << "Params: \n";
    for (const auto &param : params) {
        param->pattern->print(os, indent + 2);
        if (param->type) {
            param->type->print(os, indent + 2);
        } else {
            os << "Any";
        }
        os << "\n";
    }
    print_indent(os, indent + 1);
    os << "Return Type:\n";
    if (return_type) {
        (*return_type)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);

    os << "Body:\n";
    if (body) {
        (*body)->print(os, indent + 2);
    }
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
        os << "Field(name=" << field->name.lexeme << ")\n";
        print_indent(os, indent + 3);
        os << "Type:\n";
        field->type->print(os, indent + 4);
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
            os << "Field(name=" << field->name.lexeme << ")\n";
            print_indent(os, indent + 3);
            os << "Type:\n";
            field->type->print(os, indent + 4);
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

void TraitDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "TraitDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Associated Items:\n";
    for (const auto &item : associated_items) {
        item->print(os, indent + 2);
    }
}

void ImplBlock::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "ImplBlock\n";
    if (trait_name) {
        print_indent(os, indent + 1);
        os << "Trait Name:\n";
        (*trait_name)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "Target Type:\n";
    target_type->print(os, indent + 2);
    print_indent(os, indent + 1);
    os << "Implemented Items:\n";
    for (const auto &item : implemented_items) {
        item->print(os, indent + 2);
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
    if (generic_args) {
        print_indent(os, indent + 1);
        os << "Generic Arguments:\n";
        for (const auto &arg : *generic_args) {
            arg->print(os, indent + 2);
        }
    }
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
    os << "Mutability: " << (is_mutable ? "true" : "false") << "\n";
    print_indent(os, indent + 1);
    os << "Referenced Type:\n";
    referenced_type->print(os, indent + 2);
}

void SliceTypeNode::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "SliceTypeNode\n";
    print_indent(os, indent + 1);
    os << "Element Type:\n";
    element_type->print(os, indent + 2);
}

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
void print_struct_pattern_field(const std::shared_ptr<StructPatternField> &field, std::ostream &os,
                                int indent) {
    print_indent(os, indent);
    os << "StructPatternField(name=" << field->field_name.lexeme << ")\n";
    if (field->pattern) {
        print_indent(os, indent + 1);
        os << "Pattern:\n";
        (*field->pattern)->print(os, indent + 2);
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

void RestPattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "RestPattern(..)\n";
}

void SlicePattern::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "SlicePattern\n";
    print_indent(os, indent + 1);
    os << "Elements:\n";
    for (const auto &element : elements) {
        element->print(os, indent + 2);
    }
}

void FnParam::print(std::ostream &os, int indent) const {}

void Field::print(std::ostream &os, int indent) const {}

void FieldInitializer::print(std::ostream &os, int indent) const {}

void StructPatternField::print(std::ostream &os, int indent) const {}

// Expression accept methods - use ExprVisitor
std::shared_ptr<Symbol> LiteralExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> ArrayLiteralExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> ArrayInitializerExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> VariableExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> UnaryExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> BinaryExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> CallExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> IfExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> LoopExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> WhileExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> IndexExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> FieldAccessExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> AssignmentExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> CompoundAssignmentExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> ReferenceExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> UnderscoreExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> StructInitializerExpr::accept(ExprVisitor *visitor) {
    return visitor->visit(this);
}
std::shared_ptr<Symbol> UnitExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> GroupingExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> TupleExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> AsExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> MatchExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> PathExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }
std::shared_ptr<Symbol> BlockExpr::accept(ExprVisitor *visitor) { return visitor->visit(this); }

// Statement accept methods
void BlockStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void ExprStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void LetStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void ReturnStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void BreakStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void ContinueStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }
void ItemStmt::accept(StmtVisitor *visitor) { visitor->visit(this); }

// Type node accept methods
void TypeNameNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void ArrayTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void UnitTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void TupleTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void PathTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void RawPointerTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void ReferenceTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void SliceTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }
void SelfTypeNode::accept(TypeVisitor *visitor) { visitor->visit(this); }

// Item accept methods
void FnDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void StructDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void ConstDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void EnumDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void ModDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void TraitDecl::accept(ItemVisitor *visitor) { visitor->visit(this); }
void ImplBlock::accept(ItemVisitor *visitor) { visitor->visit(this); }

// Pattern accept methods
void IdentifierPattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void WildcardPattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void LiteralPattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void TuplePattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void SlicePattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void StructPattern::accept(PatternVisitor *visitor) { visitor->visit(this); }
void RestPattern::accept(PatternVisitor *visitor) { visitor->visit(this); }

// Other accept methods
void MatchArm::accept(OtherVisitor *visitor) { visitor->visit(this); }
void FnParam::accept(OtherVisitor *visitor) { visitor->visit(this); }
void StructPatternField::accept(OtherVisitor *visitor) { visitor->visit(this); }
void Field::accept(OtherVisitor *visitor) { visitor->visit(this); }
void FieldInitializer::accept(OtherVisitor *visitor) { visitor->visit(this); }
void EnumVariant::accept(OtherVisitor *visitor) { visitor->visit(this); }

void Program::accept(ProgramVisitor *visitor) { visitor->visit(this); }
