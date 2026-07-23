# Enforce minimum CMake verison
cmake_minimum_required(VERSION 3.20)

# Initialize project name and other metadata
project(Key_Val_Store
    VERSION 1.0
    DESCRIPTION "concurrent in-memory key-value store with a network server"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
# make a hard error if compiler doesn't support C+++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kvstore src/main.cpp)
target_include_directories(kvstore PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_executable(bench benchmarks/bench_main.cpp)
target_include_directories(bench PRIVATE ${CMAKE_SOURCE_DIR}/src)

find_package(GTest REQUIRED)
enable_testing()
add_executable(store_tests tests/store_test.cpp)
target_include_directories(store_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(store_tests GTest::GTest GTest::Main)
add_test(NAME store_tests COMMAND store_tests)
