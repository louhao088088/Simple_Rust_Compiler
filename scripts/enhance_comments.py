#!/usr/bin/env python3
import re
import os

def clean_header(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    new_lines = []
    for line in lines:
        if '//' in line and not line.strip().startswith('/**') and not line.strip().startswith('*'):
            code_part = line.split('//')[0].rstrip()
            if code_part.strip():
                new_lines.append(code_part + '\n')
        else:
            new_lines.append(line)
    
    with open(filepath, 'w') as f:
        f.writelines(new_lines)
    print(f"Cleaned: {filepath}")

def main():
    for fname in os.listdir('src/ir'):
        if fname.endswith('.h'):
            clean_header(os.path.join('src/ir', fname))
    print("Done!")

if __name__ == '__main__':
    main()
