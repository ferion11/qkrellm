# SPDX-License-Identifier: GPL-3.0-or-later
# Layering (dependencies point down only), see docs/architecture.md:
#   app -> ui -> core -> sensors      (pluginhost joins in M4)
add_subdirectory(sensors)
add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)
