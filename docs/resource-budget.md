# Resource budget

qkrellm exists to be light. These numbers are requirements (ADR 0006), checked by
`benchmarks/measure.sh --check` and partly by CTest. Defaults live in
`benchmarks/budget.env`; changing one needs an ADR with measurements.

| Metric | Limit | How measured |
|---|---|---|
| Private memory at idle | ≤ 16 MiB | `Private_Dirty` in `/proc/<pid>/smaps_rollup` after 2 s settle |
| Idle CPU | ≤ 0.2 % (2 ‰) of one core, default monitors (ADR 0013) | `/proc/<pid>/schedstat` delta over the window (ns) |
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

## After bitmap text (M1, 2026-11-23, same machine and build type)

| Platform | startup | private | PSS | RSS | CPU |
|---|---|---|---|---|---|
| offscreen | 21 ms | 1.9 MiB | 11.3 MiB | 29 MiB | 0 ‰ |
| xcb | 44 ms | 2.5 MiB | 12.3 MiB | 31 MiB | 0 ‰ |

## End of M1 (2026-11-23): CPU panel, sampling at 1 Hz, release build, 30 s window

| Platform | startup | private | PSS | RSS | CPU |
|---|---|---|---|---|---|
| offscreen | 19 ms | 2.2 MiB | 11.4 MiB | 29 MiB | 0 ‰ |
| xcb | 39 ms | 2.7 MiB | 12.6 MiB | 32 MiB | 0 ‰ |

## M2 memory panel (2026-11-23): CPU + Mem panels (no swap on this machine), same setup

| Platform | startup | private | PSS | RSS | CPU |
|---|---|---|---|---|---|
| offscreen | 20 ms | 2.2 MiB | 11.4 MiB | 29 MiB | 0 ‰ |
| xcb | 43 ms | 2.7 MiB | 12.6 MiB | 32 MiB | 0 ‰ |
| xcb, + Swap panel | 36 ms | 2.7 MiB | 12.6 MiB | 32 MiB | 1 ‰ |
| xcb, 70 px, + 2 network panels | 35 ms | 2.7 MiB | 12.6 MiB | 32 MiB | 1 ‰ |
| xcb, + Disk panel | 42 ms | 2.7 MiB | 12.7 MiB | 32 MiB | 1 ‰ |
| xcb, + hwmon (Temp chart + 6 rows) | 47 ms | 2.7 MiB | 12.8 MiB | 32 MiB | 1 ‰ |

The CPU column above came from clock ticks rounded down (a "1 ‰" there means 1.0–1.9 ‰).
From here on CPU is measured exactly from `/proc/<pid>/schedstat` (ADR 0013):

| Setup (release, 60 s window) | startup | private | CPU |
|---|---|---|---|
| main before battery (precise re-measure) | 45 ms | 2.7 MiB | 1.56 ‰ (xcb) |
| + battery, /sys/block disks, hwmon tiers, header/chart repaint skipping, chart scrolling | 46 ms | 2.8 MiB | 1.0 ‰ (offscreen), 1.2–1.4 ‰ (xcb) |
| GKrellM 2 on the same machine, same method (for reference) | — | 6.2 MiB | 6.1 ‰ |

Cold per-read costs at 1 Hz (thread CPU time): /proc/stat 137 µs, /proc/diskstats 100 µs vs
/sys/block/<disk>/stat 10–16 µs, /proc/net/dev 34 µs, /proc/meminfo 28 µs, coretemp 57 µs,
battery uevent 357 µs (first read each second refreshes ACPI; any attribute costs that),
AC adapter 93 µs. Hot-loop measurements are 3–5× lower and misleading for a 1 Hz app.

hwmon read costs on the reference laptop (per read): coretemp ~5 µs CPU; acpitz ~55 µs;
iwlwifi ~17 µs CPU / 150 µs wall; Asus WMI fans ~0.6 ms CPU; NVMe 3–15 ms wall
(~0.1 ms CPU). Reading everything every second measured 4 ‰ CPU, hence the slow-sensor
period in `HwmonMonitor`.

M0 findings (before bitmap text):
- ~41 MiB of RSS is file-backed and shared: libQt6Gui/Widgets/Core (~17 MiB),
  two fontconfig caches (~8 MiB), locale archive (~2 MiB), libstdc++.

## Text rendering investigation (M1, 2026-11-23)

Machine: 6 240 installed fonts. Details and decision: ADR 0011.

| Scenario (one 11 px string) | first text ready | private memory |
|---|---|---|
| no text | — (first paint ≈ 20 ms) | 1.7 MiB |
| Qt + system fonts | 117 ms offscreen / 130 ms xcb | 11.5 MiB |
| Qt + minimal fontconfig (DejaVu only) | 17 ms | 2.6 MiB |
| bitmap glyph strip (`drawPixmap`) | 3 ms | 1.95 MiB |

- The M0 "~9.8 MiB heap" and "~120 ms first drawText" are the same cause: Qt populating its
  font database from fontconfig (≈ 87 ms) + font matching (≈ 28 ms). heaptrack: 5.6 of
  7.06 MiB peak heap allocated inside libfontconfig, one allocation per installed font.
- Rasterising text is ≈ 0.3 ms; syscalls ≈ 14 ms in total — the cost is CPU.
- Cost scales with the user's installed fonts, so it cannot be budgeted unless panels avoid
  system fonts → bitmap atlases by default (ADR 0011).
- Implemented in M1: see "After bitmap text" above.

Reproduce with the standalone experiment in `benchmarks/text-cost/` (usage in its
`main.cpp`; modes `notext`, `system`, `appfont`, `gen`, `bitmap`), and profile the app with
`heaptrack -o /tmp/qkrellm/bench/heap <binary> --exit-after-first-paint`.
