# Thread/CMakeLists.txt

file(GLOB SOURCES "*.cc")

add_library(Thread STATIC ${SOURCES})
target_include_directories(Thread PUBLIC ${PROJECT_SOURCE_DIR}/Thread)

# Thread 库依赖 base
target_link_libraries(Thread PRIVATE base)