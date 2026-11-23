# Changelog

All notable changes to this project are documented here. Format:
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/); versioning:
[SemVer](https://semver.org/) for the application. The plugin ABI has its own version
(`QK_ABI_VERSION_MAJOR.MINOR`), reported in a separate "Plugin ABI" subsection.

## [Unreleased]

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

### Fixed
- Build with Qt 6.5 (minimum): `tst_textatlas` used `QByteArray(std::string)`, which only
  exists in newer Qt. `scripts/check-qt-min.sh` now reproduces the CI minimum-Qt job locally.

### Changed
- Text rendering strategy decided (ADR 0011): panels will use bitmap glyph atlases by
  default; system fonts become opt-in. Investigation numbers in `docs/resource-budget.md`.

### Plugin ABI
- v0.1 draft: `qk_host_api`, `qk_plugin_descriptor`, `qk_plugin_entry`; C example plugin;
  header-only C++ wrapper. Unstable until M4.
