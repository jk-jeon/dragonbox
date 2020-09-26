cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# TODO: specify version
project(dragonbox
        VERSION 0.0.1
        LANGUAGES CXX)

# ---- Includes ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# ---- Install directories ----

set(dragonbox_directory "dragonbox-${PROJECT_VERSION}")
set(dragonbox_include_directory "${CMAKE_INSTALL_INCLUDEDIR}/${dragonbox_directory}")

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(dragonbox_warning_guard "SYSTEM")
if(dragonbox_INCLUDE_WITHOUT_SYSTEM)
  set(dragonbox_warning_guard "")
endif()

# ---- Declare library (dragonbox) ----

add_library(dragonbox INTERFACE)
add_library(dragonbox::dragonbox ALIAS dragonbox)

set(dragonbox_headers include/dragonbox/dragonbox.h)

target_include_directories(dragonbox
        ${dragonbox_warning_guard}
        INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${dragonbox_include_directory}>")

target_compile_features(dragonbox INTERFACE cxx_std_17)

# ---- Declare library (dragonbox_to_chars) ----

set(dragonbox_to_chars_headers
        ${dragonbox_headers}
        include/dragonbox/dragonbox_to_chars.h)

set(dragonbox_to_chars_sources source/dragonbox_to_chars.cpp)

add_library(dragonbox_to_chars STATIC
        ${dragonbox_to_chars_headers}
        ${dragonbox_to_chars_sources})
add_library(dragonbox::dragonbox_to_chars ALIAS dragonbox_to_chars)

target_include_directories(dragonbox_to_chars
        ${dragonbox_warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${dragonbox_include_directory}>")

target_compile_features(dragonbox_to_chars PUBLIC cxx_std_17)

# ---- Install ----

install(TARGETS dragonbox dragonbox_to_chars
        EXPORT dragonboxTargets
        RUNTIME #
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT dragonbox_Runtime
        LIBRARY #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT dragonbox_Runtime
        NAMELINK_COMPONENT dragonbox_Development
        ARCHIVE #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT dragonbox_Development)

set(dragonbox_install_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/${dragonbox_directory}")

write_basic_package_version_file(
        dragonboxConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

install(EXPORT dragonboxTargets
        NAMESPACE dragonbox::
        DESTINATION "${dragonbox_install_cmakedir}")

install(FILES
        "${PROJECT_SOURCE_DIR}/cmake/dragonboxConfig.cmake"
        "${PROJECT_BINARY_DIR}/dragonboxConfigVersion.cmake"
        DESTINATION "${dragonbox_install_cmakedir}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION "${dragonbox_include_directory}")
