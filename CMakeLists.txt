cmake_minimum_required(VERSION 3.25)
project(chess_intelligence_system LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_TLS_VERIFY OFF)

# Enable testing
include(CTest)
enable_testing()

# Add engine subdirectory
add_subdirectory(engine)
