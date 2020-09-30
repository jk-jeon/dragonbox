cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(benchmark LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(dragonbox SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_Declare(ryu SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/ryu")
FetchContent_Declare(schubfach SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/schubfach")
FetchContent_Declare(grisu_exact SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/grisu_exact")
FetchContent_MakeAvailable(dragonbox ryu schubfach grisu_exact)

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
