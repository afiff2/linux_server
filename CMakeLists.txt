cmake_minimum_required(VERSION 3.14)
project(MyNetworkProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加头文件搜索路径（让各模块互相引用）
include_directories(${PROJECT_SOURCE_DIR}/base ${PROJECT_SOURCE_DIR})

# 添加子模块
add_subdirectory(Logger)
add_subdirectory(Thread)
add_subdirectory(net)
add_subdirectory(base)