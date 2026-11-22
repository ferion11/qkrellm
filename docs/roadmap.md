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

## M1 — Vertical slice: CPU

- [ ] **Investigate first-text-draw cost (~120 ms)** and private heap (~9.8 MiB) with
      heaptrack; decide text strategy (ADR)
- [ ] `src/sensors/`: `qkrellm_sensors` target; `/proc/stat` parser + fixtures (≥ 2 kernels) + fuzzer
- [ ] `core`: `RingBuffer`, `Sampler` (single coarse timer), `MetricRegistry`
- [ ] `ui`: `ChartWidget`, `Panel`; CPU panel visible in MainWindow
- [ ] Budget still met (`measure.sh --check`); CI runs the fuzzer for 60 s

## M2 — Built-in monitors

- [ ] Memory/swap (`/proc/meminfo`)
- [ ] Network (`/proc/net/dev`), per-interface
- [ ] Disk I/O (`/proc/diskstats`)
- [ ] hwmon temperatures/fans/voltages (`/sys/class/hwmon`)
- [ ] Battery (`/sys/class/power_supply`)
- [ ] Filesystems (`statvfs` on `/proc/mounts` entries)
- [ ] Clock / uptime / load

## M3 — Configuration and themes

- [ ] Config schema + migrations (QSettings INI), preferences dialog
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

1. Maintainer: `gh auth login`, review branch `chore/m0-foundation`, push and check CI.
2. Start M1 with the text-cost investigation (it shapes UI code), then the `/proc/stat`
   vertical slice using the `new-monitor` skill.
