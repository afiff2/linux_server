cmake_minimum_required(VERSION 3.14)
project(ClientApp)

set(SRC_FILES
    client.cc
)

add_executable(client ${SRC_FILES})

target_link_libraries(client
    PRIVATE
        net
        Logger
        Thread
        base
        pthread
        rt
)