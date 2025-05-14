add_library(EventLoop STATIC
    EventLoop.cc
)

# 确保 EventLoop 链接 BaseUtils（因为它用了 Logger）
target_link_libraries(EventLoop PRIVATE BaseUtils)

add_subdirectory(test)