# 0005 — Linux only, read /proc and /sys directly

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

Supporting several kernels multiplies backends and CI. libsensors/libudev add
dependencies and memory for data we can read from sysfs ourselves.

## Decision

- Linux is the only target. No portability layer beyond what keeps code tidy.
- Sensors read `/proc` and `/sys` (hwmon, power_supply, net, block) directly.
- Parsing is split from I/O: **parsers are pure functions over text** (`std::string_view`
  in, plain structs/`std::optional` out) in `src/sensors/`; readers keep files open and use
  `pread` into preallocated buffers.

## Consequences

- Parsers are unit-tested against byte-exact fixtures in `tests/fixtures/proc/<tag>/` and
  fuzzed with libFuzzer.
- Kernel format differences must be handled by tolerant parsers, with one fixture tag per
  format variant.
