# net/CMakeLists.txt

file(GLOB SOURCES "*.cc" "test/*.cc")

add_library(net STATIC ${SOURCES})
target_include_directories(net PUBLIC ${PROJECT_SOURCE_DIR}/net)

# net 库依赖 base Thread 和 Logger
target_link_libraries(net PRIVATE base Thread Logger)

# 添加测试程序
add_subdirectory(test)