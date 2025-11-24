#!/bin/bash

# 精简 IR 生成器 .cpp 文件中的注释脚本

IR_DIR="/home/louhao/compiler/src/ir"

# 处理所有 .cpp 文件
for file in "$IR_DIR"/*.cpp; do
    echo "处理文件: $file"
    
    # 创建临时文件
    tmp_file="${file}.tmp"
    
    # 使用 sed 删除以下类型的注释：
    # 1. 删除文件头部的多行注释块 (/** ... */)
    # 2. 删除分隔线注释 // ==========
    # 3. 删除显而易见的单行注释
    sed -i.bak \
        -e '/^\/\*\*/,/\*\//d' \
        -e '/^[[:space:]]*\/\/[[:space:]]*=[=]*[[:space:]]*$/d' \
        -e '/^[[:space:]]*\/\/[[:space:]]*[0-9]\+\.[[:space:]]/d' \
        -e 's/[[:space:]]*\/\/[[:space:]]*\(进入\|退出\|分配\|存储\|加载\|计算\|获取\|生成\|输出\|调用\).*//' \
        "$file"
    
    echo "  完成: $file"
done

echo "所有文件处理完成！"
