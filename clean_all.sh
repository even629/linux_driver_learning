#!/bin/bash

# 遍历当前目录下的所有子目录
for dir in */; do
    # 检查是否真的是目录（防止符号链接等异常情况）
    if [ -d "$dir" ]; then
        echo "enter DIR: $dir"
        (
            cd "$dir" || exit
            if [ -f Makefile ] || [ -f makefile ]; then
                echo "execute make clean..."
                make clean
            else
                echo "warning: jump $dir because no Makefile was found。"
            fi
        )
        echo "leaving: $dir"
        echo "--------------------------"
    fi
done

echo "all done"
