# SPDX-License-Identifier: GPL-3.0-or-later
qt_add_executable(qkrellm
    main.cpp
    startupprobe.cpp
    startupprobe.h)
target_link_libraries(qkrellm PRIVATE qkrellm_ui)
qk_set_warnings(qkrellm)

install(TARGETS qkrellm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
