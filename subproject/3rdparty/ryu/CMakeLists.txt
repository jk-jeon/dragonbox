cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ryu_cmake LANGUAGES C)

include(FetchContent)
FetchContent_Declare(ryu_cmake
        GIT_REPOSITORY https://github.com/ulfjack/ryu.git
        GIT_TAG aa31ca9361d21b1a00ee054aac49c87d07e74abc)
FetchContent_GetProperties(ryu_cmake)
if(NOT ryu_cmake_POPULATED)
  FetchContent_Populate(ryu_cmake)
endif()

# Globbing is safe if the git tag above is not changed between configures
file(GLOB ryu_headers "${ryu_cmake_SOURCE_DIR}/ryu/*.h")
file(GLOB ryu_sources "${ryu_cmake_SOURCE_DIR}/ryu/*.c")

add_library(ryu STATIC ${ryu_headers} ${ryu_sources})
add_library(ryu::ryu ALIAS ryu)

target_include_directories(ryu SYSTEM PUBLIC "${ryu_cmake_SOURCE_DIR}")
