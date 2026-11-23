# SPDX-License-Identifier: GPL-3.0-or-later
# qkrellm_ui: QWidgets + QPainter only (ADR 0003 — no QML/Quick).
add_library(qkrellm_ui STATIC
    bitmaptextrenderer.cpp
    bitmaptextrenderer.h
    chartwidget.cpp
    chartwidget.h
    mainwindow.cpp
    mainwindow.h
    panel.cpp
    panel.h
    systemfonttextrenderer.cpp
    systemfonttextrenderer.h
    textatlas.cpp
    textatlas.h
    textformat.cpp
    textformat.h
    textrenderer.h
    theme.h)
target_link_libraries(qkrellm_ui
    PUBLIC
        qkrellm_core
        Qt6::Gui
        Qt6::Widgets)
qk_set_warnings(qkrellm_ui)

# Bitmap glyph atlases (ADR 0011), available as :/qkrellm/fonts/<name>.{png,qkatlas}.
qt_add_resources(qkrellm_ui "qkrellm_fonts"
    PREFIX "/qkrellm/fonts"
    BASE fonts
    FILES
        fonts/default.png
        fonts/default.qkatlas
        fonts/default@2x.png
        fonts/default@2x.qkatlas)
