# SPDX-License-Identifier: GPL-3.0-or-later
# Global instrumentation switches (sanitizers, coverage). Applied to every target.

if(QKRELLM_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

if(QKRELLM_ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
    add_link_options(--coverage)
endif()

if(QKRELLM_ENABLE_FUZZ AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "QKRELLM_ENABLE_FUZZ requires clang (use the 'fuzz' preset).")
endif()
