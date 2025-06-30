cmake_minimum_required(VERSION 3.16)
project(AdvancedErrorCorrectionCodes VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_options(/arch:AVX2)  # Enable SIMD
    add_compile_options(/std:c++20)  # C++20 support
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-march=native)  # Enable SIMD
    add_compile_options(-static-libgcc -static-libstdc++)
    add_compile_options(-fconcepts)  # C++20 concepts
    add_link_options(-static-libgcc -static-libstdc++)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find packages
find_package(Threads REQUIRED)

# Optional packages for advanced features
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
    add_compile_definitions(HAVE_OPENMP)
endif()

# Include directories
include_directories(include)

# Directory variables
set(SRC_DIR src)
set(TEST_DIR test)
set(EXAMPLES_DIR examples)
set(TESTS_DIR tests)
set(BENCHMARKS_DIR benchmarks)
set(OUTPUT_DIR output)

# Library sources
set(LIB_SOURCES
    ${SRC_DIR}/hamming_code.cpp
    ${SRC_DIR}/bch_code.cpp
    ${SRC_DIR}/reed_solomon.cpp
    ${SRC_DIR}/ldpc_code.cpp
    ${SRC_DIR}/turbo_code.cpp
    ${SRC_DIR}/galois_field.cpp
    ${SRC_DIR}/matrix_operations.cpp
    ${SRC_DIR}/performance_analyzer.cpp
    ${SRC_DIR}/error_simulator.cpp
)

# Create static library
add_library(ecc_lib STATIC ${LIB_SOURCES})
target_link_libraries(ecc_lib Threads::Threads)
if(OpenMP_CXX_FOUND)
    target_link_libraries(ecc_lib OpenMP::OpenMP_CXX)
endif()

# Main executable
add_executable(ecc_demo ${SRC_DIR}/main.cpp)
target_link_libraries(ecc_demo ecc_lib)

# Test executable
add_executable(ecc_test
    ${TESTS_DIR}/test_hamming.cpp
    ${TESTS_DIR}/test_bch.cpp
    ${TESTS_DIR}/test_reed_solomon.cpp
    ${TESTS_DIR}/test_performance.cpp
    ${TESTS_DIR}/test_main.cpp
)
target_link_libraries(ecc_test ecc_lib)

# Benchmark executable
add_executable(ecc_benchmark
    ${BENCHMARKS_DIR}/benchmark_codes.cpp
    ${BENCHMARKS_DIR}/ber_analysis.cpp
)
target_link_libraries(ecc_benchmark ecc_lib)

# BER Analysis executable
add_executable(ber_analysis_test
    ${EXAMPLES_DIR}/ber_analysis_test.cpp
)
target_link_libraries(ber_analysis_test ecc_lib)

# Installation
install(TARGETS ecc_demo ecc_test ecc_benchmark ber_analysis_test
        RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS ecc_lib ARCHIVE DESTINATION lib)
