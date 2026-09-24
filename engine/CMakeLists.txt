# Language standard and build type come from the root CMakeLists.txt
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(CHESS_NATIVE "Optimize for the building machine (-march=native)" OFF)

# Core engine library
add_library(chess_core STATIC
    src/board/bitboard.cpp
    src/board/fen.cpp
    src/board/position.cpp
    src/board/types.cpp
    src/board/zobrist.cpp
    src/evaluation/evaluate.cpp
    src/move/attacks.cpp
    src/move/move.cpp
    src/move/movegen.cpp
    src/move/perft.cpp
    src/protocol/uci.cpp
    src/search/search.cpp
    src/search/tt.cpp
)

target_include_directories(chess_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(Threads REQUIRED)
target_link_libraries(chess_core PUBLIC Threads::Threads)

if(CHESS_NATIVE AND NOT MSVC)
    target_compile_options(chess_core PUBLIC -march=native)
endif()

# Engine CLI / UCI executable
add_executable(chess_engine src/main.cpp)
target_link_libraries(chess_engine PRIVATE chess_core)

# Benchmark and perft gate (non-zero exit on perft mismatch)
add_executable(chess_bench bench/bench.cpp)
target_link_libraries(chess_bench PRIVATE chess_core)

include(CheckIPOSupported)
check_ipo_supported(RESULT CHESS_IPO_SUPPORTED OUTPUT CHESS_IPO_OUTPUT LANGUAGES CXX)
if(CHESS_IPO_SUPPORTED)
    set_property(TARGET chess_core chess_engine chess_bench PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
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

    add_executable(chess_tests
        tests/perft/test_perft.cpp
        tests/search/test_tactics.cpp
        tests/unit/test_attacks.cpp
        tests/unit/test_bitboard.cpp
        tests/unit/test_evaluate.cpp
        tests/unit/test_fen.cpp
        tests/unit/test_infra.cpp
        tests/unit/test_make_unmake.cpp
        tests/unit/test_move.cpp
        tests/unit/test_movegen.cpp
        tests/unit/test_position.cpp
        tests/unit/test_search.cpp
        tests/unit/test_tt.cpp
        tests/unit/test_types.cpp
        tests/unit/test_uci.cpp
        tests/unit/test_zobrist.cpp
    )
    target_link_libraries(chess_tests PRIVATE chess_core GTest::gtest_main)
    if(CHESS_IPO_SUPPORTED)
        # Must match chess_core or its LTO objects fail to link on Clang + GNU ld
        set_property(TARGET chess_tests PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif()

    include(GoogleTest)
    gtest_discover_tests(chess_tests)
endif()
