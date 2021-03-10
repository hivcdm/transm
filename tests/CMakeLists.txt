#
# CMakeLists.txt

# Build file for the tests
#
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

enable_testing()
find_package(GTest CONFIG REQUIRED)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(
	tests
	main-test.cpp
)

add_test(NAME check COMMAND tests)

target_include_directories (
    tests
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link against Google Test libraries 
# NOTICE: For correct linking :: has to be used see here: https://github.com/google/googletest/tree/master/googletest)
target_link_libraries(tests GTest::gtest GTest::gmock)
