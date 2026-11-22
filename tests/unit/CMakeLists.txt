# SPDX-License-Identifier: GPL-3.0-or-later
# qk_add_unit_test(<name> SOURCES ... [LIBS ...]) — one Qt Test executable per test file.
function(qk_add_unit_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    target_compile_definitions(${name} PRIVATE
        QK_FIXTURES_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures")
    qk_set_warnings(${name})
    add_test(NAME unit.${name} COMMAND ${name})
    set_tests_properties(unit.${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

qk_add_unit_test(tst_version
    SOURCES tst_version.cpp
    LIBS qkrellm_core)
target_compile_definitions(tst_version PRIVATE QK_EXPECTED_VERSION="${PROJECT_VERSION}")
