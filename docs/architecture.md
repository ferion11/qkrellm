# Architecture

qkrellm is one process, one thread for UI and sampling, one top-level window.

## Layers

Dependencies point **down only**. A layer never includes headers from a layer above it.

```
 src/app/         main(), CLI, window-manager hints, single instance        (Qt Widgets)
    │
 src/ui/          MainWindow, Panel, ChartWidget, Krell, Decal, Theme        (Qt Gui/Widgets)
    │
 src/pluginhost/  dlopen loader, C ABI <-> core adapter, fault isolation     (QtCore)   [M4]
    │
 src/core/        Sampler (one coarse QTimer), MetricRegistry, RingBuffer,   (QtCore only)
    │             Config (QSettings), version
 src/sensors/     pure parsers (string_view -> struct) + thin Linux readers  (no Qt in parsers) [M1]

 include/qkrellm/ public plugin ABI (C11) + plugin.hpp — depends on nothing
 plugins/         example plugins, built only against include/qkrellm
```

CMake targets mirror this: `qkrellm_core` (links Qt6::Core only), `qkrellm_ui` (adds
Gui/Widgets), `qkrellm` executable. `qkrellm_sensors` and `qkrellm_pluginhost` appear in
M1 and M4.

## Data flow (target design, M1+)

```
QTimer (CoarseTimer, default 1 Hz)
  └─> Sampler::tick()
        ├─> each Monitor reads its fds with pread() into a preallocated buffer
        ├─> pure parser -> sample struct
        ├─> delta/rate computation -> RingBuffer<float, N> per metric (no allocation)
        └─> emits changed(metricId) only when a value changed
              └─> Panel::update(dirtyRect)  (no full-window repaints)
```

Rules that keep this cheap:
- **No allocation in the sampling hot path.** Buffers and ring buffers are sized at
  construction. `perf-reviewer` agent checks this.
- **Files stay open.** `/proc/stat` etc. are opened once; `pread(fd, buf, n, 0)` re-reads.
- **One timer.** Monitors declare a period that is a multiple of the base tick.
- **Repaint only what changed**; static theme parts are cached `QPixmap`s.

## Plugins (M4)

Plugins are shared objects found in `$XDG_DATA_HOME/qkrellm/plugins` and
`<prefix>/lib/qkrellm/plugins`. The host `dlopen`s them, resolves `qk_plugin_entry`,
checks `abi_major`, then calls `create()` with a `qk_host_api`. Plugins provide values;
the host renders them with the same widgets as built-in monitors. See `plugin-api.md` and
ADR 0004.

## Testing strategy

| Level | What | Where |
|---|---|---|
| Unit | parsers vs. byte-exact kernel fixtures, ring buffer, config migrations | `tests/unit/` (Qt Test) |
| Fuzz | every parser | `tests/fuzz/` (libFuzzer, preset `fuzz`) |
| ABI | C11 consumer + layout asserts + `dlopen`; C++ wrapper; C plugin has no Qt/libstdc++ | `tests/abi/` |
| Smoke | real binary starts, paints, exits (`--exit-after-first-paint`) | `tests/CMakeLists.txt` |
| Budget | linked Qt modules; startup/private memory/CPU | `budget.qt_modules`, `benchmarks/measure.sh` |
| Static | clang-format, clang-tidy, clazy, cppcheck | `scripts/lint.sh` |
