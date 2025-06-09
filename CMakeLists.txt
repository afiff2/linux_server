cmake_minimum_required(VERSION 3.14)
project(Linux_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#----------------------------------------------
# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
# endif()

# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#   message(STATUS "=== Debug build: enabling AddressSanitizer ===")
#   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -g")
#   # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,leak,undefined -fno-omit-frame-pointer -g")
# endif()
#----------------------------------------------


add_subdirectory(Logger)
add_subdirectory(Thread)
add_subdirectory(net)
add_subdirectory(base)

add_subdirectory(app)