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

qk_add_unit_test(tst_textatlas
    SOURCES tst_textatlas.cpp
    LIBS qkrellm_ui)
target_compile_definitions(tst_textatlas PRIVATE QK_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# ENABLE_EXPORTS lets the test's FcFontList() interpose libfontconfig's (ADR 0011 probe).
qk_add_unit_test(tst_bitmaptext
    SOURCES tst_bitmaptext.cpp
    LIBS qkrellm_ui ${CMAKE_DL_LIBS})
set_target_properties(tst_bitmaptext PROPERTIES ENABLE_EXPORTS ON)

qk_add_unit_test(tst_procstat
    SOURCES tst_procstat.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_diskstats
    SOURCES tst_diskstats.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_hwmon
    SOURCES tst_hwmon.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_meminfo
    SOURCES tst_meminfo.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_netdev
    SOURCES tst_netdev.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_powersupply
    SOURCES tst_powersupply.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_procfile
    SOURCES tst_procfile.cpp
    LIBS qkrellm_sensors Qt6::Core)

qk_add_unit_test(tst_ringbuffer
    SOURCES tst_ringbuffer.cpp
    LIBS qkrellm_core)

qk_add_unit_test(tst_sampling
    SOURCES tst_sampling.cpp
    LIBS qkrellm_core)

qk_add_unit_test(tst_panels
    SOURCES tst_panels.cpp
    LIBS qkrellm_ui ${CMAKE_DL_LIBS})
set_target_properties(tst_panels PROPERTIES ENABLE_EXPORTS ON)
