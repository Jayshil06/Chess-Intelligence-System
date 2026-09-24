cmake_minimum_required(VERSION 3.25)
project(chess_engine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Common compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core engine library
file(GLOB_RECURSE ENGINE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/board/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/move/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/search/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/evaluation/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/nnue/*.cpp"
)

# If no source files exist yet in subdirectories, make sure a minimal file is present
if(NOT ENGINE_SOURCES)
    set(ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/board/types.cpp")
endif()

add_library(chess_core STATIC ${ENGINE_SOURCES})

target_include_directories(chess_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(Threads REQUIRED)
target_link_libraries(chess_core PUBLIC Threads::Threads)

# Engine CLI / UCI executable
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(chess_engine "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    target_link_libraries(chess_engine PRIVATE chess_core)
endif()

# Testing infrastructure with GoogleTest
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        URL_HASH SHA256=f179ec217f9b3b3f3c6e8b02d3e7eda997b49e4ce26d6b235c9053bec9c0bf9f
        FIND_PACKAGE_ARGS NAMES GTest
    )
    # For Windows: prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    if(TEST_SOURCES)
        add_executable(chess_tests ${TEST_SOURCES})
        target_link_libraries(chess_tests PRIVATE chess_core GTest::gtest_main)
        target_include_directories(chess_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

        include(GoogleTest)
        gtest_discover_tests(chess_tests)
    endif()
endif()
