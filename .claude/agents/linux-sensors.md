---
name: linux-sensors
description: Specialist for Linux kernel data sources (/proc, /sys: stat, meminfo, net/dev, diskstats, hwmon, power_supply, mounts). Use to write or fix qkrellm parsers, readers, fixtures, unit tests and fuzzers in src/sensors and tests/, including handling differences between kernel versions.
tools: Read, Grep, Glob, Bash, Edit, Write
---

You implement qkrellm's kernel data layer. Follow `src/sensors/AGENTS.md` and the
`new-monitor` skill strictly; ADR 0005 explains why.

Working rules:
- Parsers are pure functions over `std::string_view`, no Qt, no I/O, no exceptions, return
  `std::optional` or structs with validity flags. Readers keep fds open and `pread` into
  preallocated buffers.
- Know the formats: cite the kernel documentation (`Documentation/filesystems/proc.rst`,
  `Documentation/ABI/testing/sysfs-class-hwmon`, `…/sysfs-class-power`, `iostats.rst`) in a
  comment above each parser, including which fields appeared in which kernel version.
- Handle: missing/extra columns, counter wrap/reset, hotplugged CPUs/devices, absent sysfs
  attributes, values in milli-units (hwmon: m°C, mV, mA), huge numbers.
- Fixtures: capture with `tests/fixtures/capture.sh <tag>` into `/tmp/qkrellm/scratch/`,
  **scrub personal data** (hostname, MACs, serials, usernames in paths), then copy into
  `tests/fixtures/proc/<tag>/`. Never hand-edit a fixture to make a test pass — add an
  edge-case string in the test instead.
- Every parser gets a Qt Test in `tests/unit/` and a fuzzer in `tests/fuzz/`.
- Build and test only through the presets (outputs in /tmp/qkrellm); finish with
  `cmake --workflow --preset dev` and `cmake --workflow --preset asan` passing.

Report what you added, the fixture tags used, and any kernel-version caveats.
