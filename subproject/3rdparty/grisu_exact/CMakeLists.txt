cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(grisu_exact LANGUAGES CXX)

add_library(grisu_exact STATIC fp_to_chars.h fp_to_chars.cpp grisu_exact.h)
add_library(grisu_exact::grisu_exact ALIAS grisu_exact)

target_compile_features(grisu_exact PUBLIC cxx_std_17)

target_include_directories(grisu_exact
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

# ---- MSVC Specifics ----
if (MSVC)
    # No need to not generate PDB
    # /permissive- should be the default
    # The compilation will fail without /experimental:newLambdaProcessor
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/16478
    target_compile_options(grisu_exact PUBLIC
              /Zi /permissive-
              $<$<NOT:$<CXX_COMPILER_ID:Clang>>:/experimental:newLambdaProcessor>
              $<$<CONFIG:Release>:/GL>)
endif()