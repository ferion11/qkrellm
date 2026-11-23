# Architecture Decision Records

One file per decision, numbered, never rewritten after acceptance — supersede instead.
Create new ones with the `adr` skill (or copy `0000-template.md`) and add a row here.

| # | Decision | Status |
|---|---|---|
| [0001](0001-record-architecture-decisions.md) | Record architecture decisions as ADRs | Accepted |
| [0002](0002-cpp20-cmake-qt-modules.md) | C++20, CMake presets, Qt ≥ 6.5 limited to Core/Gui/Widgets | Accepted |
| [0003](0003-qwidgets-qpainter-ui.md) | UI with QWidgets + QPainter, no QML | Accepted |
| [0004](0004-stable-c-plugin-abi.md) | Stable versioned C plugin ABI with header-only C++ wrapper | Accepted |
| [0005](0005-linux-only-procfs-sysfs.md) | Linux only, read /proc and /sys directly | Accepted |
| [0006](0006-resource-budget-is-a-requirement.md) | The resource budget is a tested requirement | Accepted |
| [0007](0007-config-and-theme-formats.md) | INI config via QSettings; own simple theme format | Accepted |
| [0008](0008-wayland-window-placement.md) | Wayland window placement is an open risk, decided in M5 | Proposed |
| [0009](0009-no-gkrellm-compatibility.md) | No GKrellM theme or gkrellmd protocol compatibility | Accepted |
| [0010](0010-out-of-tree-builds-in-tmp.md) | All local build/test/bench output lives in /tmp/qkrellm | Accepted |
| [0011](0011-bitmap-text-by-default.md) | Panels use bitmap glyph atlases; system fonts opt-in | Accepted |
| [0012](0012-chart-autoscale-power-of-two.md) | Rate charts autoscale to a power-of-two ceiling | Accepted |
