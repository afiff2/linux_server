# net/test/CMakeLists.txt

find_package(Threads REQUIRED)

add_executable(test1 test1.cc)
add_executable(test2 test2.cc)
add_executable(test_logger test_logger.cc)
add_executable(test3 test3.cc)

# 公共依赖项
set(TEST_LIBS_PRIVATE
    net
    Thread
    Logger
    base
    Threads::Threads
)

# 为每个可执行文件链接相同的依赖
target_link_libraries(test1 PRIVATE ${TEST_LIBS_PRIVATE})
target_link_libraries(test2 PRIVATE ${TEST_LIBS_PRIVATE})
target_link_libraries(test3 PRIVATE ${TEST_LIBS_PRIVATE})
target_link_libraries(test_logger PRIVATE ${TEST_LIBS_PRIVATE})