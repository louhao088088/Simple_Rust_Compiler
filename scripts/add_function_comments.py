#!/usr/bin/env python3
"""
Add English function comments to C++ source files and remove inline comments.
"""

import re
import sys

# Function comment templates
FUNCTION_COMMENTS = {
    'visit(LiteralExpr': '// Generate IR for literal expressions (numbers, booleans, chars, strings).',
    'visit(VariableExpr': '// Generate IR for variable expressions (load from local or global variables).',
    'visit(BinaryExpr': '// Generate IR for binary expressions (arithmetic, comparison, logical operators).',
    'visit(UnaryExpr': '// Generate IR for unary expressions (-, !, &, *).',
    'visit(CallExpr': '// Generate IR for function call expressions.',
    'visit(AssignmentExpr': '// Generate IR for assignment expressions.',
    'visit(GroupingExpr': '// Generate IR for grouping expressions (parentheses).',
    'visit(BlockExpr': '// Generate IR for block expressions.',
    'visit(AsExpr': '// Generate IR for type cast expressions (as).',
    'visit(ReferenceExpr': '// Generate IR for reference expressions (&expr).',
    'visit(CompoundAssignmentExpr': '// Generate IR for compound assignment expressions (+=, -=, *=, /=, %=).',
    'visit(ArrayLiteralExpr': '// Generate IR for array literal expressions.',
    'visit(ArrayInitializerExpr': '// Generate IR for array initializer expressions ([value; size]).',
    'visit(ArrayIndexExpr': '// Generate IR for array index expressions (arr[idx]).',
    'visit(StructInitializerExpr': '// Generate IR for struct initializer expressions.',
    'visit(FieldAccessExpr': '// Generate IR for field access expressions (struct.field).',
    'visit(BlockStmt': '// Generate IR for block statements (scope management and sequential execution).',
    'visit(ExprStmt': '// Generate IR for expression statements.',
    'visit(LetStmt': '// Generate IR for let statements (variable declarations with initialization).',
    'visit(ItemStmt': '// Generate IR for item statements (struct declarations inside functions).',
    'visit(ReturnStmt': '// Generate IR for return statements.',
    'visit(IfExpr': '// Generate IR for if expressions with conditional branching and PHI nodes.',
    'visit(WhileExpr': '// Generate IR for while loop expressions.',
    'visit(LoopExpr': '// Generate IR for infinite loop expressions.',
    'visit(BreakExpr': '// Generate IR for break expressions.',
    'visit(ContinueExpr': '// Generate IR for continue expressions.',
    'generate(Program': '// Generate LLVM IR code from the AST program.',
    'visit_item(': '// Dispatch to appropriate visitor based on item type.',
    'visit_function_decl(': '// Generate IR for a function declaration with parameters and body.',
    'visit_struct_decl(': '// Generate IR for struct type declarations.',
    'visit_const_decl(': '// Generate IR for const declarations.',
    'visit_impl_block(': '// Generate IR for impl blocks.',
    'emit_builtin_declarations(': '// Emit declarations for C library functions used by built-in functions.',
    'get_type_size(': '// Get the size in bytes of a type.',
    'get_type_alignment(': '// Get the alignment requirement of a type.',
    'is_zero_initializer(': '// Check if an expression is a zero initializer.',
    'visit_logical_binary_expr(': '// Generate IR for short-circuit logical binary expressions (&& and ||).',
}

def process_file(filepath):
    """Process a single file."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Remove block comments at file start
    content = re.sub(r'^/\*\*.*?\*/', '', content, flags=re.DOTALL)
    
    # Remove single-line comments inside functions (but keep code)
    lines = content.split('\n')
    new_lines = []
    
    for line in lines:
        # Remove trailing // comments but keep code
        if '//' in line:
            code_part = line.split('//')[0]
            if code_part.strip():
                new_lines.append(code_part.rstrip())
            elif not line.strip().startswith('//'):
                new_lines.append(line)
        else:
            new_lines.append(line)
    
    content = '\n'.join(new_lines)
    
    # Add function comments
    for func_pattern, comment in FUNCTION_COMMENTS.items():
        # Find function definitions
        pattern = r'\n((?:std::string|void|size_t|bool|int)\s+IRGenerator::' + re.escape(func_pattern) + r'[^{]*\{)'
        
        def add_comment(match):
            func_def = match.group(1)
            # Check if comment already exists
            lines_before = content[:match.start()].split('\n')
            if lines_before and lines_before[-1].strip().startswith('//'):
                return '\n' + func_def
            return '\n' + comment + '\n' + func_def
        
        content = re.sub(pattern, add_comment, content)
    
    # Remove multiple consecutive blank lines
    content = re.sub(r'\n\n\n+', '\n\n', content)
    
    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Processed: {filepath}")

def main():
    files = [
        'src/ir/ir_generator_main.cpp',
        'src/ir/ir_generator_expressions.cpp',
        'src/ir/ir_generator_statements.cpp',
        'src/ir/ir_generator_control_flow.cpp',
        'src/ir/ir_generator_complex_exprs.cpp',
        'src/ir/ir_generator_builtins.cpp',
        'src/ir/ir_generator_helpers.cpp',
    ]
    
    for filepath in files:
        try:
            process_file(filepath)
        except Exception as e:
            print(f"Error processing {filepath}: {e}")
    
    print("\nDone!")

if __name__ == '__main__':
    main()
