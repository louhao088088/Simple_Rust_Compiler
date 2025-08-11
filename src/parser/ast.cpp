// ast.cpp
#include "ast.h"

#include <string>

// Helper function for printing indentation
static void print_indent(std::ostream &os, int indent) { os << std::string(indent * 2, ' '); }

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
    os << "LetStmt(name=" << name.lexeme << ", mut=" << (is_mutable ? "true" : "false") << ")\n";
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

void FnDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "FnDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Params: ";
    for (const auto &p : params) {
        os << p.lexeme << " ";
    }

    os << "\n";
    os << "Param Types:\n";
    for (const auto &type : param_types) {
        type->print(os, indent + 1);
    }
    os << "Return Type:\n";
    if (return_type) {
        (*return_type)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "Body:\n";
    body->print(os, indent + 2);
}

void Program::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "Program\n";
    for (const auto &item : items) {
        item->print(os, indent + 1);
    }
}

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
