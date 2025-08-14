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
    os << "Mutiability: " << (is_mutable ? "true" : "false") << "\n";
    print_indent(os, indent + 1);
    os << "Expression:\n";
    expression->print(os, indent + 2);
}

void RangeExpr::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "RangeExpr\n";
    print_indent(os, indent + 1);
    os << "Start:\n";
    if (start) {
        (*start)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "End:\n";
    if (end) {
        (*end)->print(os, indent + 2);
    }
    print_indent(os, indent + 1);
    os << "Is Inclusive: " << (is_inclusive ? "true" : "false") << "\n";
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

// ===================== Accept Method Implementations =====================

void LiteralExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayLiteralExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayInitializerExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void VariableExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void UnaryExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BinaryExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void CallExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void IfExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void LoopExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void WhileExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void IndexExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void FieldAccessExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void AssignmentExpr::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BlockStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ExprStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void LetStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ReturnStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void BreakStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ContinueStmt::accept(Visitor* visitor) {
    visitor->visit(this);
}

void TypeNameNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void ArrayTypeNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void UnitTypeNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void TupleTypeNode::accept(Visitor* visitor) {
    visitor->visit(this);
}

void FnDecl::accept(Visitor* visitor) {
    visitor->visit(this);
}

void Program::accept(Visitor* visitor) {
    visitor->visit(this);
}

// Pattern accept implementations
void IdentifierPattern::accept(Visitor* visitor) {
    visitor->visit(this);
}

// ===================== Semantic Analysis Implementations =====================

// SymbolTable implementation
void SymbolTable::enter_scope() {
    scopes_.emplace_back();
}

void SymbolTable::exit_scope() {
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void SymbolTable::define(const std::string& name, std::shared_ptr<Symbol> symbol) {
    if (!scopes_.empty()) {
        scopes_.back()[name] = symbol;
    }
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}

// ErrorReporter implementation
void ErrorReporter::report_error(const std::string& message, int line, int column) {
    has_errors_ = true;
    std::cerr << "Error";
    if (line >= 0) std::cerr << " at line " << line;
    if (column >= 0) std::cerr << ", column " << column;
    std::cerr << ": " << message << std::endl;
}

void ErrorReporter::report_warning(const std::string& message, int line, int column) {
    std::cerr << "Warning";
    if (line >= 0) std::cerr << " at line " << line;
    if (column >= 0) std::cerr << ", column " << column;
    std::cerr << ": " << message << std::endl;
}

// NameResolutionVisitor implementation
NameResolutionVisitor::NameResolutionVisitor(ErrorReporter& error_reporter) 
    : error_reporter_(error_reporter) {
    symbol_table_.enter_scope(); // Global scope
}

void NameResolutionVisitor::visit(LiteralExpr* node) {
    // Literals don't need symbol resolution
}

void NameResolutionVisitor::visit(ArrayLiteralExpr* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(ArrayInitializerExpr* node) {
    node->value->accept(this);
    node->size->accept(this);
}

void NameResolutionVisitor::visit(VariableExpr* node) {
    auto symbol = symbol_table_.lookup(node->name.lexeme);
    if (!symbol) {
        error_reporter_.report_error("Undefined variable '" + node->name.lexeme + "'", 
                                   node->name.line, node->name.column);
    } else {
        node->resolved_symbol = symbol;
    }
}

void NameResolutionVisitor::visit(UnaryExpr* node) {
    node->right->accept(this);
}

void NameResolutionVisitor::visit(BinaryExpr* node) {
    node->left->accept(this);
    node->right->accept(this);
}

void NameResolutionVisitor::visit(CallExpr* node) {
    node->callee->accept(this);
    for (auto& arg : node->arguments) {
        arg->accept(this);
    }
}

void NameResolutionVisitor::visit(IfExpr* node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
}

void NameResolutionVisitor::visit(LoopExpr* node) {
    node->body->accept(this);
}

void NameResolutionVisitor::visit(WhileExpr* node) {
    node->condition->accept(this);
    node->body->accept(this);
}

void NameResolutionVisitor::visit(IndexExpr* node) {
    node->object->accept(this);
    node->index->accept(this);
}

void NameResolutionVisitor::visit(FieldAccessExpr* node) {
    node->object->accept(this);
}

void NameResolutionVisitor::visit(AssignmentExpr* node) {
    node->target->accept(this);
    node->value->accept(this);
}

void NameResolutionVisitor::visit(BlockStmt* node) {
    symbol_table_.enter_scope();
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    if (node->final_expr) {
        (*node->final_expr)->accept(this);
    }
    symbol_table_.exit_scope();
}

void NameResolutionVisitor::visit(ExprStmt* node) {
    node->expression->accept(this);
}

void NameResolutionVisitor::visit(LetStmt* node) {
    if (node->initializer) {
        (*node->initializer)->accept(this);
    }
    
    // Handle pattern binding
    if (node->pattern) {
        node->pattern->accept(this);
    }
}

void NameResolutionVisitor::visit(ReturnStmt* node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void NameResolutionVisitor::visit(BreakStmt* node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void NameResolutionVisitor::visit(ContinueStmt* node) {
    // No symbols to resolve
}

void NameResolutionVisitor::visit(TypeNameNode* node) {
    // Type resolution would be handled here
}

void NameResolutionVisitor::visit(ArrayTypeNode* node) {
    node->element_type->accept(this);
    node->size->accept(this);
}

void NameResolutionVisitor::visit(UnitTypeNode* node) {
    // Unit type has no symbols
}

void NameResolutionVisitor::visit(TupleTypeNode* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(FnDecl* node) {
    // Define function symbol
    auto fn_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::FUNCTION);
    symbol_table_.define(node->name.lexeme, fn_symbol);
    node->resolved_symbol = fn_symbol;
    
    // Process function body with new scope
    if (node->body) {
        symbol_table_.enter_scope();
        // TODO: Define parameter symbols
        (*node->body)->accept(this);
        symbol_table_.exit_scope();
    }
}

void NameResolutionVisitor::visit(Program* node) {
    for (auto& item : node->items) {
        item->accept(this);
    }
}

// Pattern visitors (simplified implementations)
void NameResolutionVisitor::visit(IdentifierPattern* node) {
    // Define the variable symbol
    auto var_symbol = std::make_shared<Symbol>(node->name.lexeme, Symbol::VARIABLE);
    symbol_table_.define(node->name.lexeme, var_symbol);
}

void NameResolutionVisitor::visit(WildcardPattern* node) {
    // Wildcard patterns don't bind variables
}

void NameResolutionVisitor::visit(LiteralPattern* node) {
    // Literal patterns don't bind variables
}

void NameResolutionVisitor::visit(TuplePattern* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(SlicePattern* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void NameResolutionVisitor::visit(StructPattern* node) {
    // TODO: Handle struct pattern fields
}

void NameResolutionVisitor::visit(RestPattern* node) {
    // Rest patterns don't bind variables themselves
}

// TypeCheckVisitor implementation
TypeCheckVisitor::TypeCheckVisitor(ErrorReporter& error_reporter) 
    : error_reporter_(error_reporter) {}

void TypeCheckVisitor::visit(LiteralExpr* node) {
    // TODO: Infer type from literal token
}

void TypeCheckVisitor::visit(ArrayLiteralExpr* node) {
    // TODO: Type check array elements and infer array type
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(ArrayInitializerExpr* node) {
    node->value->accept(this);
    node->size->accept(this);
    // TODO: Type check and infer array type
}

void TypeCheckVisitor::visit(VariableExpr* node) {
    if (node->resolved_symbol && node->resolved_symbol->type) {
        node->type = node->resolved_symbol->type;
    }
}

void TypeCheckVisitor::visit(UnaryExpr* node) {
    node->right->accept(this);
    // TODO: Type check unary operations
}

void TypeCheckVisitor::visit(BinaryExpr* node) {
    node->left->accept(this);
    node->right->accept(this);
    // TODO: Type check binary operations
}

void TypeCheckVisitor::visit(CallExpr* node) {
    node->callee->accept(this);
    for (auto& arg : node->arguments) {
        arg->accept(this);
    }
    // TODO: Type check function call
}

void TypeCheckVisitor::visit(IfExpr* node) {
    node->condition->accept(this);
    node->then_branch->accept(this);
    if (node->else_branch) {
        (*node->else_branch)->accept(this);
    }
    // TODO: Type check if expression
}

void TypeCheckVisitor::visit(LoopExpr* node) {
    node->body->accept(this);
    // TODO: Loop expressions return ()
}

void TypeCheckVisitor::visit(WhileExpr* node) {
    node->condition->accept(this);
    node->body->accept(this);
    // TODO: While expressions return ()
}

void TypeCheckVisitor::visit(IndexExpr* node) {
    node->object->accept(this);
    node->index->accept(this);
    // TODO: Type check array/index access
}

void TypeCheckVisitor::visit(FieldAccessExpr* node) {
    node->object->accept(this);
    // TODO: Type check field access
}

void TypeCheckVisitor::visit(AssignmentExpr* node) {
    node->target->accept(this);
    node->value->accept(this);
    // TODO: Type check assignment compatibility
}

void TypeCheckVisitor::visit(BlockStmt* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    if (node->final_expr) {
        (*node->final_expr)->accept(this);
    }
}

void TypeCheckVisitor::visit(ExprStmt* node) {
    node->expression->accept(this);
}

void TypeCheckVisitor::visit(LetStmt* node) {
    if (node->initializer) {
        (*node->initializer)->accept(this);
    }
    // TODO: Type check let statement
}

void TypeCheckVisitor::visit(ReturnStmt* node) {
    if (node->value) {
        (*node->value)->accept(this);
        // TODO: Check return type compatibility
    }
}

void TypeCheckVisitor::visit(BreakStmt* node) {
    if (node->value) {
        (*node->value)->accept(this);
    }
}

void TypeCheckVisitor::visit(ContinueStmt* node) {
    // Continue statements don't have types
}

void TypeCheckVisitor::visit(TypeNameNode* node) {
    // Type nodes don't need type checking
}

void TypeCheckVisitor::visit(ArrayTypeNode* node) {
    node->element_type->accept(this);
    node->size->accept(this);
}

void TypeCheckVisitor::visit(UnitTypeNode* node) {
    // Unit type is always valid
}

void TypeCheckVisitor::visit(TupleTypeNode* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(FnDecl* node) {
    if (node->body) {
        // TODO: Set current return type for return statements
        (*node->body)->accept(this);
    }
}

void TypeCheckVisitor::visit(Program* node) {
    for (auto& item : node->items) {
        item->accept(this);
    }
}

// Pattern visitors for TypeCheckVisitor (simplified implementations)
void TypeCheckVisitor::visit(IdentifierPattern* node) {
    // Pattern type checking would go here
}

void TypeCheckVisitor::visit(WildcardPattern* node) {
    // No type checking needed for wildcard
}

void TypeCheckVisitor::visit(LiteralPattern* node) {
    // Type check literal pattern
}

void TypeCheckVisitor::visit(TuplePattern* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(SlicePattern* node) {
    for (auto& element : node->elements) {
        element->accept(this);
    }
}

void TypeCheckVisitor::visit(StructPattern* node) {
    // TODO: Handle struct pattern type checking
}

void TypeCheckVisitor::visit(RestPattern* node) {
    // No specific type checking for rest patterns
}

void ItemStmt::print(std::ostream &os, int indent) const { item->print(os, indent); }

void FnDecl::print(std::ostream &os, int indent) const {
    print_indent(os, indent);
    os << "FnDecl(name=" << name.lexeme << ")\n";
    print_indent(os, indent + 1);
    os << "Params: \n";
    for (const auto &param : params) {
        param.pattern->print(os, indent + 2);
        if (param.type) {
            param.type->print(os, indent + 2);
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