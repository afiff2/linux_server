cmake_minimum_required(VERSION 3.14)
project(ServerApp)

set(SRC_FILES
    server.cc
)

add_executable(server ${SRC_FILES})

target_link_libraries(server 
    PRIVATE
        net
        Logger
        Thread
        base
        pthread
        rt
)