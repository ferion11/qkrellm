# Resource budget

qkrellm exists to be light. These numbers are requirements (ADR 0006), checked by
`benchmarks/measure.sh --check` and partly by CTest. Defaults live in
`benchmarks/budget.env`; changing one needs an ADR with measurements.

| Metric | Limit | How measured |
|---|---|---|
| Private memory at idle | ≤ 16 MiB | `Private_Dirty` in `/proc/<pid>/smaps_rollup` after 2 s settle |
| Idle CPU | ≤ 0.1 % of one core | utime+stime delta over the window (default 10 s) |
| Startup | ≤ 150 ms | process start (`/proc/self/stat` starttime) → first paint, best of 5 |
| Linked Qt modules | Core, Gui, Widgets only | `readelf -d` (CTest `budget.qt_modules`) |
| Sampling hot path | zero heap allocations | review (`perf-reviewer`), heaptrack spot checks |

RSS and PSS are logged for information; they are dominated by shared Qt/fontconfig pages.

## Running

```sh
cmake --preset release && cmake --build --preset release
benchmarks/measure.sh --check                 # offscreen
QT_QPA_PLATFORM=xcb benchmarks/measure.sh     # real X11 session
heaptrack -o /tmp/qkrellm/bench/heap /tmp/qkrellm/build/release/src/app/qkrellm
valgrind --tool=massif --massif-out-file=/tmp/qkrellm/bench/massif.out \
    /tmp/qkrellm/build/release/src/app/qkrellm --exit-after-first-paint
```

History: `/tmp/qkrellm/bench/results.tsv` (date, commit, startup, private, pss, rss, cpu).

## Baseline (M0, 2026-11-22, Qt 6.11.3, GCC 16, MinSizeRel+LTO, Gentoo x86_64)

| Platform | startup | private | PSS | RSS | CPU |
|---|---|---|---|---|---|
| offscreen | 129–132 ms | 11.5 MiB | 22.6 MiB | 53 MiB | 0 ‰ |
| xcb | 158–170 ms | 12.0 MiB | 23.7 MiB | 55 MiB | 0 ‰ |

Findings:
- ~41 MiB of RSS is file-backed and shared: libQt6Gui/Widgets/Core (~17 MiB),
  two fontconfig caches (~8 MiB), locale archive (~2 MiB), libstdc++.
- ~9.8 MiB of private memory is heap for an empty window — worth a heaptrack pass in M1.
- **The first `QPainter::drawText` costs ~120 ms** (fontconfig + glyph rasterisation): an
  experiment with the same window painted without text reached first paint in ~20 ms.
  → M1/M3 must decide the text strategy (e.g. theme-provided bitmap font strips, as
  GKrellM themes did, or deferring text until after first paint).
