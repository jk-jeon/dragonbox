cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(dragonboxTests LANGUAGES CXX)

# ---- Add root project ----

# We want warnings in tests
set(dragonbox_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(dragonbox SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(dragonbox)

# ---- Tests ----

enable_testing()

# TODO: add tests
