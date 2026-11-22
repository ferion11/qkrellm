# SPDX-License-Identifier: GPL-3.0-or-later
# ABI contract tests. They include ONLY include/qkrellm — never src/ — exactly like a
# third-party plugin would.

# 1) Pure C11 consumer: layout static_asserts + dlopen of the example plugin.
add_executable(abi_c11 abi_c11.c)
target_include_directories(abi_c11 PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(abi_c11 PRIVATE ${CMAKE_DL_LIBS})
set_target_properties(abi_c11 PROPERTIES C_STANDARD 11 C_EXTENSIONS OFF AUTOMOC OFF)
qk_set_warnings(abi_c11)
add_test(NAME abi.c11_load_example COMMAND abi_c11 "$<TARGET_FILE:qkrellm_example_hello>")
add_dependencies(abi_c11 qkrellm_example_hello)

# A C plugin must not drag in the C++ runtime or Qt.
add_test(NAME abi.c_plugin_has_no_cxx_or_qt
    COMMAND "${PROJECT_SOURCE_DIR}/scripts/check-qt-modules.sh"
            "$<TARGET_FILE:qkrellm_example_hello>" --forbid-cxx)

# 2) C++ wrapper (plugin.hpp) compiles and behaves under C++20 with all warnings.
add_executable(abi_cpp abi_cpp.cpp)
set_target_properties(abi_cpp PROPERTIES AUTOMOC OFF)
target_include_directories(abi_cpp PRIVATE "${PROJECT_SOURCE_DIR}/include")
qk_set_warnings(abi_cpp)
add_test(NAME abi.cpp_wrapper COMMAND abi_cpp)
