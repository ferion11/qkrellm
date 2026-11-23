# src/sensors — kernel data parsers and readers

Contents (ADR 0005), one `.h/.cpp` pair each:
- `textscan` — shared allocation-free tokenizing; `sysfsvalue` — one-integer sysfs files.
- `procstat` + `cpuusage` — /proc/stat and CPU usage deltas.
- `meminfo` — /proc/meminfo and memory usage derivation.
- `netdev` — /proc/net/dev; `rate` — `counterRate` for monotonically increasing counters.
- `diskstats` — /proc/diskstats and `parseBlockStat` for /sys/block/<disk>/stat (what the
  disk monitor reads: much cheaper).
- `hwmon` — startup discovery of /sys/class/hwmon inputs.
- `powersupply` — power_supply uevents and the battery estimate.
- `ProcFile` — the only per-tick I/O: open once, `pread`. (`hwmon` discovery and the
  monitors' startup scans also touch the filesystem, once.)

Library `qkrellm_sensors` links **no Qt**. Rules:

## Split parsing from I/O

- **Parser**: pure function, no I/O, no Qt, no allocation if avoidable:
  `std::optional<CpuTimes> parseProcStat(std::string_view text);`
  Put each parser in its own `.h/.cpp` (`procstat.h`, `meminfo.h`, …).
- **Reader**: owns a file descriptor opened once (`O_RDONLY | O_CLOEXEC`) and re-reads
  with `pread(fd, buf, size, 0)` into a buffer allocated at construction. Readers are the
  only code here that touches the filesystem.

## Robustness

- Tolerate missing, extra and reordered fields; kernels add columns over time.
- Return `std::nullopt` (or a partially-filled struct with validity flags) — never throw,
  never `abort`, never read past the `string_view`.
- Counters wrap and reset (CPU hotplug, interface down/up, suspend): rate computations
  must handle `now < previous`.
- Sysfs values can be absent per device; discover devices at startup and on demand, not
  every tick.

## Tests are mandatory

1. Capture: `tests/fixtures/capture.sh <tag>` (or `capture-sys.sh <tag>` for sysfs
   hwmon trees) → writes to `/tmp/qkrellm/scratch/<tag>/`.
2. **Scrub** hostnames, MACs, serials, user paths; copy needed files to
   `tests/fixtures/proc/<tag>/…` (byte-exact; `.gitattributes` keeps them untouched).
3. Unit test in `tests/unit/tst_<parser>.cpp` covering every fixture tag plus hand-made
   edge cases (empty, truncated, garbage, huge numbers).
4. Fuzzer `tests/fuzz/fuzz_<parser>.cpp`, registered in `tests/fuzz/CMakeLists.txt` and
   `scripts/fuzz.sh` (CI fuzzes every parser for 60 s).

The `linux-sensors` agent and the `new-monitor` skill follow this recipe.
