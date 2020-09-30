cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(dragonbox_meta LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(dragonbox SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_Declare(common SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../common")
FetchContent_MakeAvailable(dragonbox common)

function(meta_exe NAME)
  add_executable(${NAME} source/${NAME}.cpp)

  target_compile_features(${NAME} PRIVATE cxx_std_17)

  target_link_libraries(${NAME} PRIVATE ${ARGN})
endfunction()

meta_exe(generate_cache dragonbox::common)
meta_exe(generate_compressed_cache_error_table dragonbox::dragonbox)
meta_exe(live_test dragonbox::dragonbox_to_chars)
meta_exe(perf_test dragonbox::common dragonbox::dragonbox_to_chars)
meta_exe(sandbox dragonbox::dragonbox)
