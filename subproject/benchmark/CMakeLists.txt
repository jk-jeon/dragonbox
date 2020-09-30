cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(benchmark LANGUAGES CXX)

set(benchmark_headers include/benchmark.h)

set(benchmark_sources
        source/dragonbox.cpp
        source/grisu_exact.cpp
        source/main.cpp
        source/ryu.cpp
        source/schubfach.cpp)

add_executable(benchmark ${benchmark_headers} ${benchmark_sources})

target_compile_features(benchmark PRIVATE cxx_std_17)

target_include_directories(benchmark
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_link_libraries(benchmark
        PRIVATE
        ryu::ryu
        dragonbox::dragonbox_to_chars
        grisu_exact::grisu_exact
        schubfach::schubfach_32
        schubfach::schubfach_64)
