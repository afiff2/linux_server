# Logger/CMakeLists.txt

file(GLOB SOURCES "*.cc")

add_library(Logger STATIC ${SOURCES})
target_include_directories(Logger PUBLIC ${PROJECT_SOURCE_DIR}/Logger)

target_link_libraries(Logger PRIVATE base Thread)