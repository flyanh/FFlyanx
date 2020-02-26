# 设置CMake的最小版本需求
cmake_minimum_required(VERSION 3.9)
# 设置工程名称
project(Flyanx)
# 设置C语言所使用的标准
set(CMAKE_C_STANDARD 90)
# 导入头文件目录，特别是针对于操作系统工程
include_directories(HEAD "./include")

# 将所有的可执行文件添加进跟踪列表
add_executable(
        Flyanx ./src/kernel/main.c ./src/kernel/protect.c ./src/kernel/table.c ./src/kernel/start.c
)
