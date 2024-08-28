cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(dragonbox_simple LANGUAGES CXX)

add_library(dragonbox_simple INTERFACE include/simple_dragonbox.h)
add_library(dragonbox::simple ALIAS dragonbox_simple)

target_include_directories(dragonbox_simple
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_features(dragonbox_simple INTERFACE cxx_std_17)