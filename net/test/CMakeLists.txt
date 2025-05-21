# net/test/CMakeLists.txt

find_package(Threads REQUIRED)

# 所有测试程序名（.cc 文件名，不含扩展名）
set(TESTS
    test1
    test2
    test3
    test4
    test5
    test6
    test7
    test8
    test9
    test10
    test_logger
)

# 公共依赖项
set(TEST_LIBS_PRIVATE
    net
    Thread
    Logger
    base
    Threads::Threads
)

# 对每个测试自动 add + link
foreach(test_name IN LISTS TESTS)
    add_executable(${test_name} ${test_name}.cc)
    target_link_libraries(${test_name} PRIVATE ${TEST_LIBS_PRIVATE})
endforeach()