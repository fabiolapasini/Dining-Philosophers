cmake_minimum_required(VERSION 3.21)

project(DiningPhilosophers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup()

option(BUILD_TESTS "Build tests" ON)
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")

if(BUILD_TESTS)
    enable_testing()
    # vcpkg feature opzionale (solo se usi manifest mode)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

add_subdirectory(DiningPhilosophers)
# add_subdirectory(Tests)