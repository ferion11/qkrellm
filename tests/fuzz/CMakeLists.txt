# SPDX-License-Identifier: GPL-3.0-or-later
# Built only with -DQKRELLM_ENABLE_FUZZ=ON (preset 'fuzz', clang). Each parser in
# src/sensors gets one fuzzer: tests/fuzz/fuzz_<parser>.cpp defining LLVMFuzzerTestOneInput.
#
#   qk_add_fuzzer(fuzz_proc_stat SOURCES fuzz_proc_stat.cpp LIBS qkrellm_sensors)
#
# Run:  /tmp/qkrellm/build/fuzz/tests/fuzz/fuzz_proc_stat \
#           -max_total_time=60 /tmp/qkrellm/fuzz/corpus/proc_stat tests/fixtures/proc
function(qk_add_fuzzer name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${ARG_SOURCES})
    target_compile_options(${name} PRIVATE -fsanitize=fuzzer)
    target_link_options(${name} PRIVATE -fsanitize=fuzzer)
    target_link_libraries(${name} PRIVATE ${ARG_LIBS})
endfunction()

# No parsers exist yet (M0). The first one arrives in M1 with src/sensors/procstat.*.
