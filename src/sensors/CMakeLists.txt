# SPDX-License-Identifier: GPL-3.0-or-later
# qkrellm_sensors: kernel data parsers and readers (ADR 0005). No Qt at all — parsers are
# pure functions; ProcFile is the only code that touches the filesystem.
add_library(qkrellm_sensors STATIC
    cpuusage.cpp
    cpuusage.h
    diskstats.cpp
    diskstats.h
    hwmon.cpp
    hwmon.h
    meminfo.cpp
    meminfo.h
    netdev.cpp
    netdev.h
    procfile.cpp
    procfile.h
    procstat.cpp
    procstat.h
    rate.cpp
    rate.h
    sysfsvalue.cpp
    sysfsvalue.h
    textscan.cpp
    textscan.h)
set_target_properties(qkrellm_sensors PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(qkrellm_sensors PUBLIC "${PROJECT_SOURCE_DIR}/src")
qk_set_warnings(qkrellm_sensors)
