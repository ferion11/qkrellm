# SPDX-License-Identifier: GPL-3.0-or-later
# qkrellm_core: sampling, metric registry, config. Depends on QtCore only — never on Gui/Widgets.
configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/qkrellm/core/version.h" @ONLY)

add_library(qkrellm_core STATIC
    metricregistry.cpp
    metricregistry.h
    monitor.h
    monitors/batterymonitor.cpp
    monitors/batterymonitor.h
    monitors/cpumonitor.cpp
    monitors/cpumonitor.h
    monitors/diskmonitor.cpp
    monitors/diskmonitor.h
    monitors/filesystemmonitor.cpp
    monitors/filesystemmonitor.h
    monitors/hwmonmonitor.cpp
    monitors/hwmonmonitor.h
    monitors/memorymonitor.cpp
    monitors/memorymonitor.h
    monitors/netmonitor.cpp
    monitors/netmonitor.h
    monitors/systemmonitor.cpp
    monitors/systemmonitor.h
    ringbuffer.h
    sampler.cpp
    sampler.h
    version.cpp
    version.h)
target_include_directories(qkrellm_core
    PUBLIC
        "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(qkrellm_core PUBLIC Qt6::Core qkrellm_sensors)
qk_set_warnings(qkrellm_core)
