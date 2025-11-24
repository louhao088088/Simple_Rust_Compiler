#!/usr/bin/env python3
"""
Clean inline comments from C++ source files in src/ir directory.
Only keeps function-level English comments at the beginning of each function.
"""

import re
import os
import sys

def process_file(filepath):
    """Process a single C++ file to remove inline comments."""
    
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    new_lines = []
    in_function = False
    function_started = False
    brace_count = 0
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # Skip empty lines at start of file
        if not new_lines and not stripped:
            continue
            
        # Keep include statements and namespace declarations
        if stripped.startswith('#include') or stripped.startswith('namespace') or stripped.startswith('using'):
            new_lines.append(line)
            continue
        
        # Detect function start (simplified heuristic)
        if '::' in line and '(' in line and not stripped.startswith('//'):
            in_function = True
            function_started = False
            brace_count = 0
            new_lines.append(line)
            continue
        
        # Track braces to know when function ends
        if in_function:
            brace_count += line.count('{')
            brace_count -= line.count('}')
            
            if brace_count == 0 and function_started:
                in_function = False
                function_started = False
        
        if '{' in line and in_function:
            function_started = True
        
        # Remove single-line comments inside functions (but keep preprocessor directives)
        if in_function and function_started:
            # Remove // comments but keep code
            code_part = line
            if '//' in line:
                comment_pos = line.find('//')
                # Check if // is not inside a string literal
                before_comment = line[:comment_pos]
                # Simple check: count quotes before comment
                if before_comment.count('"') % 2 == 0:
                    code_part = line[:comment_pos].rstrip() + '\n'
            
            # Only add line if it's not just a comment line
            if code_part.strip() or not stripped.startswith('//'):
                if code_part.strip():  # Don't add empty lines from removed comments
                    new_lines.append(code_part)
                elif line == '\n':  # Keep intentional blank lines
                    new_lines.append(line)
        else:
            # Outside functions, keep everything except standalone comment lines
            if not (stripped.startswith('//') and len(stripped) > 2):
                new_lines.append(line)
            elif stripped.startswith('// ') and i + 1 < len(lines) and lines[i+1].strip().startswith('void '):
                # Keep comments right before function declarations
                new_lines.append(line)
    
    # Write back to file
    with open(filepath, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)
    
    print(f"Processed: {filepath}")

def main():
    ir_dir = 'src/ir'
    
    if not os.path.exists(ir_dir):
        print(f"Directory {ir_dir} not found!")
        return 1
    
    # Process all .cpp files in src/ir
    for filename in os.listdir(ir_dir):
        if filename.endswith('.cpp'):
            filepath = os.path.join(ir_dir, filename)
            process_file(filepath)
    
    print("\nComment cleaning complete!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
