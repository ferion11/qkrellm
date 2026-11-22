# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.25)

# Refuse in-source builds: all build output lives under /tmp/qkrellm (see CMakePresets.json).
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed. Use a preset, e.g.:\n"
        "  cmake --preset dev\n"
        "Build trees live in /tmp/qkrellm/build/<preset>. "
        "Remove CMakeCache.txt and CMakeFiles/ that were just created here.")
endif()

project(qkrellm
    VERSION 0.1.0
    DESCRIPTION "Lightweight Qt6 system monitor in the spirit of GKrellM"
    HOMEPAGE_URL "https://github.com/ferion11/qkrellm"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(QKRELLM_BUILD_TESTS "Build unit and ABI tests" ON)
option(QKRELLM_BUILD_PLUGINS "Build the example plugins" ON)
option(QKRELLM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(QKRELLM_ENABLE_SANITIZERS "Build with AddressSanitizer + UBSan" OFF)
option(QKRELLM_ENABLE_COVERAGE "Build with gcov instrumentation" OFF)
option(QKRELLM_ENABLE_FUZZ "Build libFuzzer targets (requires clang)" OFF)
option(QKRELLM_USE_CCACHE "Use ccache when available" ON)
option(QKRELLM_USE_MOLD "Link with mold when available" ON)

include(QkToolchain)
include(QkWarnings)
include(QkSanitizers)
include(QkQtModules)

# ADR 0002: only Core, Gui and Widgets are allowed. Adding a module requires a new ADR.
find_package(Qt6 6.5 REQUIRED COMPONENTS ${QKRELLM_ALLOWED_QT_MODULES})
qt_standard_project_setup()

include(GNUInstallDirs)

add_subdirectory(src)

if(QKRELLM_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

if(QKRELLM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(packaging)
