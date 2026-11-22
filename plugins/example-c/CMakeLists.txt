# SPDX-License-Identifier: GPL-3.0-or-later
add_library(qkrellm_example_hello MODULE hello.c)
target_include_directories(qkrellm_example_hello PRIVATE "${PROJECT_SOURCE_DIR}/include")
set_target_properties(qkrellm_example_hello PROPERTIES
    AUTOMOC OFF  # pure C: keep Qt's autogen from injecting a C++ TU (and libstdc++)
    PREFIX ""
    OUTPUT_NAME "example-hello"
    C_VISIBILITY_PRESET hidden)
qk_set_warnings(qkrellm_example_hello)

install(TARGETS qkrellm_example_hello
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/qkrellm/plugins)
