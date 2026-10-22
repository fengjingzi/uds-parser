cmake_minimum_required(VERSION 3.10)
project(uds_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(uds_parser
    src/main.cpp
    src/isotp.cpp
    src/uds.cpp
    src/dtc.cpp
)
target_include_directories(uds_parser PRIVATE include)

enable_testing()
add_executable(test_parser
    tests/test_parser.cpp
    src/isotp.cpp
    src/uds.cpp
    src/dtc.cpp
)
target_include_directories(test_parser PRIVATE include)
add_test(NAME parser_tests COMMAND test_parser)
