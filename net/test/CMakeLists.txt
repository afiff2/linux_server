find_package(Threads REQUIRED)

add_executable(test1 test1.cc)

target_link_libraries(test1
    PRIVATE
        EventLoop
        BaseUtils
        Threads::Threads
)

add_executable(test2 test2.cc)

target_link_libraries(test2
    PRIVATE
        EventLoop
        BaseUtils
        Threads::Threads
)

add_executable(test_logger test_logger.cc)
target_link_libraries(test_logger PRIVATE BaseUtils)