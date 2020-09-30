cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(schubfach LANGUAGES CXX)

add_library(schubfach_32 STATIC schubfach_32.cc schubfach_32.h)
add_library(grisu_exact::schubfach_32 ALIAS schubfach_32)

target_compile_features(grisu_exact PUBLIC cxx_std_17)

target_include_directories(schubfach_32
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

add_library(schubfach_64 STATIC schubfach_64.cc schubfach_64.h)
add_library(grisu_exact::schubfach_64 ALIAS schubfach_64)

target_compile_features(grisu_exact PUBLIC cxx_std_17)

target_include_directories(schubfach_64
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)
