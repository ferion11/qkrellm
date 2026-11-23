# Changelog

All notable changes to this project are documented here. Format:
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versioning:
[SemVer](https://semver.org/) for the application. The plugin ABI has its own version
(`QK_ABI_VERSION_MAJOR.MINOR`), reported in a separate "Plugin ABI" subsection.

## [Unreleased]

### Changed
- Default panel width 120 → 70 px (as commonly used with GKrellM); panel text atlases are
  now DejaVu Sans Condensed 9 px (was DejaVu Sans 11 px). A bytes header that does not fit
  ("7.1G/62G") falls back to the used value alone.

### Added
- Engineering foundation (M0): CMake presets with all outputs in `/tmp/qkrellm`, hello
  window with `--exit-after-first-paint`, Qt Test/ABI/smoke tests, lint stack, resource
  budget tooling and baseline, ADRs 0001–0010, agent instructions, Claude Code skills,
  agents and hooks, GitHub CI, Gentoo live ebuild draft.

### Added
- Bitmap panel text (ADR 0011): `tools/glyphgen` atlas generator, DejaVu Sans atlases
  (ASCII + Latin-1, 1×/2×), `TextRenderer` interface with `BitmapTextRenderer` (default)
  and `SystemFontTextRenderer` (fallback/opt-in). Startup on X11 164 → 44 ms, private
  memory 12.0 → 2.5 MiB.

- `qkrellm_sensors` library (no Qt): `/proc/stat` parser, CPU usage deltas, `ProcFile`
  open-once/`pread` reader; real fixtures; libFuzzer targets for the procstat and atlas
  parsers, `scripts/fuzz.sh`, CI fuzz job.
- hwmon monitor (M2): sysfs integer parser (+ fuzzer), hwmon discovery, `HwmonMonitor`
  reading only the sensors shown (slow ones, timed on first read, every 10 s); "Temp" panel with the CPU temperature (coretemp/k10temp/
  zenpower/cpu_thermal) on a fixed 0–100 °C chart, header-only rows for other chips and
  fans; `ChartScale::Fixed` (replaces `Fraction`), "°C"/"rpm" formatters, `MetricUnit::Rpm`,
  `tests/fixtures/capture-hwmon.sh`; sysfs fixtures kept byte-exact like /proc ones.
- Disk I/O monitor (M2): `/proc/diskstats` parser (+ fuzzer), `DiskMonitor` (whole disks
  backed by a device; per-disk and aggregate read/write bytes/s), aggregate "Disk" panel
  with read + write stacked and autoscaled. `counterRate` moved to `sensors/rate.h`.
- Network monitor (M2): `/proc/net/dev` parser (+ fuzzer), `counterRate`, `NetMonitor`
  (device-backed interfaces, monotonic-clock rates), one panel per interface with rx/tx
  stacked and a power-of-two autoscaled chart labelled with its scale (ADR 0012, accepted);
  `ChartScale`, header labels clipped before the value.
- Memory/swap monitor (M2): `/proc/meminfo` parser and usage derivation (procps-ng 4
  "used"; pre-3.14 fallback), `MemoryMonitor`, fuzzer; "Mem" panel (used + cache stacked,
  "7.0G/62G" header) and a "Swap" panel when swap is configured; `formatBytes` /
  `formatBytesOf` and `ValueFormat` for panel headers.
- Continuous AppImage (x86_64): `scripts/make-appimage.sh` (pinned linuxdeploy + Qt
  plugin) and `.github/workflows/appimage.yml`, which publishes every push to `main` as the
  `continuous-master` pre-release (with zsync update information). Application icon.
- Core sampling: `RingBuffer`, `MetricRegistry`, `Monitor`, single-timer `Sampler`,
  `CpuMonitor` (aggregate and per-CPU usage). The app samples once per second and shows
  current CPU usage; a test proves steady-state sampling performs no heap allocation.
- CPU panel (M1 complete): `ChartWidget` (stacked series, newest sample at the right),
  `Panel` (label + value header), framed panel stack in `MainWindow`, built-in dark
  `Theme`. The startup benchmark now reads the clock after the whole first frame.

### Fixed
- Build with Qt 6.5 (minimum): `tst_textatlas` used `QByteArray(std::string)`, which only
  exists in newer Qt. `scripts/check-qt-min.sh` now reproduces the CI minimum-Qt job locally.

### Changed
- Text rendering strategy decided (ADR 0011): panels will use bitmap glyph atlases by
  default; system fonts become opt-in. Investigation numbers in `docs/resource-budget.md`.

### Plugin ABI
- v0.1 draft: `qk_host_api`, `qk_plugin_descriptor`, `qk_plugin_entry`; C example plugin;
  header-only C++ wrapper. Unstable until M4.
