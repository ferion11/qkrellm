# SPDX-License-Identifier: GPL-3.0-or-later
# Offline generator of bitmap glyph atlases (ADR 0011). Run via tools/glyphgen/regenerate.sh.
qt_add_executable(qkrellm-glyphgen main.cpp)
target_link_libraries(qkrellm-glyphgen PRIVATE Qt6::Gui)
qk_set_warnings(qkrellm-glyphgen)
