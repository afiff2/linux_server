cmake_minimum_required(VERSION 3.14)
project(MyNetworkProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加子模块
add_subdirectory(Logger)
add_subdirectory(Thread)
add_subdirectory(net)
add_subdirectory(base)

add_subdirectory(app)