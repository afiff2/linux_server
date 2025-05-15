# base/CMakeLists.txt

file(GLOB SOURCES "*.cc")

add_library(base STATIC ${SOURCES})
target_include_directories(base PUBLIC ${PROJECT_SOURCE_DIR}/base)