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
    set_target_properties(${name} PROPERTIES AUTOMOC OFF)
endfunction()

# Seeds: tests/fixtures (procstat, meminfo, netdev, diskstats, sysfsvalue, powersupply, mounts, loadavg) and src/ui/fonts (textatlas); scripts/fuzz.sh runs them.
qk_add_fuzzer(fuzz_procstat SOURCES fuzz_procstat.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_meminfo SOURCES fuzz_meminfo.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_mounts SOURCES fuzz_mounts.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_netdev SOURCES fuzz_netdev.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_diskstats SOURCES fuzz_diskstats.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_powersupply SOURCES fuzz_powersupply.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_sysfsvalue SOURCES fuzz_sysfsvalue.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_loadavg SOURCES fuzz_loadavg.cpp LIBS qkrellm_sensors)
qk_add_fuzzer(fuzz_textatlas SOURCES fuzz_textatlas.cpp LIBS qkrellm_ui)
