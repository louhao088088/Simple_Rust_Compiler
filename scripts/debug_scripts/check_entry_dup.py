import sys

with open('/tmp/c23.ll', 'r') as f:
    lines = f.readlines()

in_function = False
function_name = ""
entry_count = 0
line_num = 0

for i, line in enumerate(lines, 1):
    if line.startswith('define '):
        if in_function and entry_count > 1:
            print(f"函数 {function_name} 有 {entry_count} 个entry标签!")
        in_function = True
        function_name = line.split('@')[1].split('(')[0] if '@' in line else "unknown"
        entry_count = 0
    elif line.strip() == 'entry:':
        entry_count += 1
        if entry_count > 1:
            print(f"第{i}行: 函数 {function_name} 的第 {entry_count} 个entry标签")
    elif line.startswith('}') and in_function:
        if entry_count > 1:
            print(f"函数 {function_name} 总共有 {entry_count} 个entry标签")
        in_function = False

if in_function and entry_count > 1:
    print(f"最后一个函数 {function_name} 有 {entry_count} 个entry标签")
