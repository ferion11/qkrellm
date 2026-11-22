# SPDX-License-Identifier: GPL-3.0-or-later
# qkrellm_ui: QWidgets + QPainter only (ADR 0003 — no QML/Quick).
add_library(qkrellm_ui STATIC
    mainwindow.cpp
    mainwindow.h)
target_link_libraries(qkrellm_ui
    PUBLIC
        qkrellm_core
        Qt6::Gui
        Qt6::Widgets)
qk_set_warnings(qkrellm_ui)
