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

### Plugin ABI
- v0.1 draft: `qk_host_api`, `qk_plugin_descriptor`, `qk_plugin_entry`; C example plugin;
  header-only C++ wrapper. Unstable until M4.
