cmake_minimum_required(VERSION 3.21)

project(UnitTests)

find_package(GTest CONFIG REQUIRED)

enable_testing()

add_executable(unit_tests
    example_test.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)