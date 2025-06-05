cmake_minimum_required(VERSION 3.14)
project(MyNetworkProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#----------------------------------------------
# 默认用 Debug 模式，这样能编译出符号，并且把 -O0 交给编译器
# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
# endif()

# # 在 Debug 模式下追加 AddressSanitizer 选项
# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#   message(STATUS "=== Debug build: enabling AddressSanitizer ===")
#   # 把 -fsanitize=address 加到 CXX flags
#   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -g")
#   # 如果你以后想用 LeakSanitizer、UBSan，可以按需再加 -fsanitize=leak -fsanitize=undefined
#   # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,leak,undefined -fno-omit-frame-pointer -g")
# endif()
#----------------------------------------------


# 添加子模块
add_subdirectory(Logger)
add_subdirectory(Thread)
add_subdirectory(net)
add_subdirectory(base)

add_subdirectory(app)