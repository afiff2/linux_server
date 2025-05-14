cmake_minimum_required(VERSION 3.14)
project(MyNetworkLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加 base 目录
add_subdirectory(base)
add_subdirectory(net)

include_directories(${PROJECT_SOURCE_DIR}/base ${PROJECT_SOURCE_DIR}/net)