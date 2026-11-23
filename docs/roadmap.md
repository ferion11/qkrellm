# Roadmap and status

This file is the **handoff point between sessions**. Update the checkboxes and the
"Next steps" section at the end of every working session (see `AGENTS.md`).

## M0 — Engineering foundation ✅ (branch `chore/m0-foundation`)

- [x] ADRs 0001–0010, architecture, resource budget, plugin API draft
- [x] AGENTS.md / CLAUDE.md + nested instructions for `include/qkrellm` and `src/sensors`
- [x] CMake + presets (dev, dev-clang, release, asan, coverage, fuzz), outputs in /tmp/qkrellm
- [x] Hello window, `--exit-after-first-paint`, smoke test
- [x] Plugin ABI v0 stub, C example plugin, ABI tests (C11, C++ wrapper, no-Qt check)
- [x] Lint stack: clang-format, clang-tidy, clazy, cppcheck (`scripts/lint.sh`)
- [x] Budget tooling (`benchmarks/measure.sh`) and baseline numbers
- [x] Claude Code: settings, hooks, project skills, project agents
- [x] GitHub: CI workflow, PR/issue templates; CONTRIBUTING, CHANGELOG, SECURITY
- [x] Gentoo live ebuild draft in `packaging/gentoo/`
- [ ] First push + green CI (needs `gh auth login` and maintainer approval)

## M1 — Vertical slice: CPU ✅

- [x] Investigate first-text-draw cost (~120 ms) and private heap (~9.8 MiB): both are
      Qt's font database populated from fontconfig (∝ installed fonts) → ADR 0011
- [x] Bitmap text (ADR 0011): `tools/glyphgen`, committed DejaVu atlases (ASCII + Latin-1,
      1×/2×), `TextRenderer` / `BitmapTextRenderer` / `SystemFontTextRenderer`, MainWindow
      uses it, `tst_bitmaptext` proves the font database stays unpopulated.
      xcb: startup 164 → 44 ms, private 12.0 → 2.5 MiB
- [x] `src/sensors/`: `qkrellm_sensors` target (no Qt); `/proc/stat` parser, CPU usage
      deltas, `ProcFile` reader; fixtures `linux-6.18-x86_64-20cpu` (two snapshots 1 s apart);
      older column layouts (2.4 → 2.6.33) covered by edge-case tests; fuzzers for procstat
      and textatlas; `scripts/fuzz.sh`; CI fuzz job (60 s each)
- [x] Second real /proc/stat fixture: `linux-6.18.18-manjaro-28cpu` (maintainer's machine;
      same 10-column layout — older layouts stay covered by edge-case tests)
- [x] `core`: `RingBuffer`, `MetricRegistry` (handles, no alloc on push), `Monitor`,
      `Sampler` (one coarse QTimer, per-monitor periods), `CpuMonitor` (aggregate busy/user/
      system + per-CPU busy); app samples at 1 Hz and shows "CPU nn%";
      `tst_sampling` proves steady-state ticks allocate nothing (operator-new probe)
- [x] `ui`: `Theme` (built-in dark defaults until M3), `ChartWidget` (stacked series, one
      column per sample, fillRect only), `Panel` (label + value header via `TextRenderer`);
      MainWindow is a framed panel stack; CPU panel shows system + user stacked;
      `tst_panels` checks bar heights pixel-exactly and that the whole window renders without
      populating the font database
- [x] Budget met with the real CPU panel (xcb: 39 ms, 2.7 MiB private, CPU 0 ‰ over 30 s)

## M2 — Built-in monitors

- [x] Memory/swap (`/proc/meminfo`): `sensors/meminfo` (+ fuzzer), `MemoryMonitor`
      (levels pushed at construction), "Mem" panel (used + cache) and "Swap" panel (only
      when swap exists at startup); `formatBytes` ("3.2G") and `ValueFormat` in panel headers;
      fixtures `linux-6.18-x86_64-20cpu` (no swap) and `…-20cpu-swap` (2 GiB swap file)
- [x] Default width 70 px (maintainer's GKrellM width); atlases DejaVu Sans Condensed 9 px;
      bytes header falls back to "used" when "used/total" does not fit
- [ ] Network (`/proc/net/dev`), per-interface
- [ ] Disk I/O (`/proc/diskstats`)
- [ ] hwmon temperatures/fans/voltages (`/sys/class/hwmon`)
- [ ] Battery (`/sys/class/power_supply`)
- [ ] Filesystems (`statvfs` on `/proc/mounts` entries)
- [ ] Clock / uptime / load

## M3 — Configuration and themes

- [ ] Config schema + migrations (QSettings INI), preferences dialog
- [ ] Config option to select `SystemFontTextRenderer` (class exists since M1; ADR 0011)
- [ ] Theme format (`theme.ini` + images), loader, 1–2 built-in themes
- [ ] Decide SVG handling without QtSvg

## M4 — Plugin ABI v1

- [ ] `src/pluginhost/`: discovery, dlopen, version checks, fault isolation
- [ ] Metric + panel description in the ABI; host-side rendering
- [ ] Plugin config via host; threading model
- [ ] Header licence decision; freeze v1; `docs/plugin-api.md` complete

## M5 — Polish and 1.0

- [ ] Window behaviour: X11 hints (sticky, skip taskbar, always on top); ADR 0008 for Wayland
- [ ] Single instance, session restore
- [ ] Packaging: .desktop/metainfo validated, ebuild in `danrepo` overlay, release tarballs
- [ ] 1.0.0 release

## Next steps

M2 in progress, one monitor per branch with the `new-monitor` skill (fixtures from ≥ 2
machines, pure parser, fuzzer, allocation-free sampling in `tst_sampling`, a panel):

1. Network (`/proc/net/dev`), per-interface rx/tx rates — first rate metric that is not a
   fraction: decide chart autoscaling (e.g. per-chart max over the visible window, rounded
   to a "nice" value, with the scale shown in the header) and record it in an ADR.
2. Disk I/O (`/proc/diskstats`) reuses the same rate + autoscale machinery.
Open questions: per-CPU charts (aggregate only today) as a config option in M3; whether
the swap panel should appear when swap is enabled after startup.
