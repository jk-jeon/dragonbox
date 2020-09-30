cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(dragonbox_common LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(dragonbox SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(dragonbox)

add_library(dragonbox_common INTERFACE)
add_library(dragonbox::common ALIAS dragonbox_common)

target_include_directories(dragonbox_common
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_features(dragonbox_common INTERFACE cxx_std_17)

target_link_libraries(dragonbox_common INTERFACE dragonbox::dragonbox)
