cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(dragonboxTests LANGUAGES CXX)

# ---- Add dependencies ----

# We want warnings in tests
set(dragonbox_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(dragonbox SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_Declare(common SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../common")
FetchContent_Declare(ryu SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/ryu")
FetchContent_MakeAvailable(dragonbox common ryu)

# ---- Tests ----

enable_testing()

function(add_test)
  cmake_parse_arguments(TEST "TO_CHARS;RYU" "NAMESPACE;NAME" "" ${ARGN})

  if(NOT TEST_NAMESPACE)
    set(TEST_NAMESPACE main)
  endif()

  if(TEST_TO_CHARS)
    set(dragonbox dragonbox::dragonbox_to_chars)
  else()
    set(dragonbox dragonbox::dragonbox)
  endif()

  set(target "${TEST_NAMESPACE}_${TEST_NAME}")
  set(test_name "${TEST_NAMESPACE}.${TEST_NAME}")

  add_executable(${target} source/${TEST_NAME}.cpp)

  target_link_libraries(${target} PRIVATE ${dragonbox} dragonbox::common)

  if(TEST_RYU)
    target_link_libraries(${target} PRIVATE ryu::ryu)
  endif()

  target_compile_features(${target} PRIVATE cxx_std_17)

  _add_test(NAME ${test_name} COMMAND ${target})
endfunction()

add_test(NAME compute_required_cache_length)
add_test(NAME test_all_shorter_interval_cases TO_CHARS RYU)
add_test(NAME uniform_random_test TO_CHARS RYU)
add_test(NAME verify_compressed_cache)
add_test(NAME verify_fast_multiplication)
add_test(NAME verify_log_computation)
add_test(NAME verify_magic_division)
