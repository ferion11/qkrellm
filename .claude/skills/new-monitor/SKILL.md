---
name: new-monitor
description: Step-by-step recipe to add a built-in qkrellm monitor (CPU, memory, network, disk, hwmon, battery…) — fixture capture, pure parser, unit test, fuzzer, reader, core registration, UI panel and docs. Use when implementing any new data source from /proc or /sys.
---

# Add a built-in monitor

Read `src/sensors/AGENTS.md` first. Consider delegating steps 1–4 to the `linux-sensors` agent.

1. **Capture fixtures** (at least one; two if the format varies by kernel):
   `tests/fixtures/capture.sh linux-$(uname -r | cut -d- -f1)-$(uname -m)-<machine> /proc/<file>`
   Review `/tmp/qkrellm/scratch/<tag>/`, scrub personal data, copy to `tests/fixtures/proc/<tag>/`.
2. **Parser** `src/sensors/<name>.h/.cpp`: pure `std::optional<Struct> parseX(std::string_view)`.
   No Qt, no I/O, no exceptions. Tolerate unknown/missing fields.
3. **Unit test** `tests/unit/tst_<name>.cpp`: every fixture tag + edge cases (empty, truncated,
   garbage, max values, counter wrap). Register with `qk_add_unit_test`.
4. **Fuzzer** `tests/fuzz/fuzz_<name>.cpp` with `LLVMFuzzerTestOneInput` calling the parser;
   `qk_add_fuzzer(...)`. Smoke it: `cmake --preset fuzz && cmake --build --preset fuzz` then
   run 60 s with corpus in `/tmp/qkrellm/fuzz/corpus/<name>` seeded from the fixtures.
5. **Reader**: opens the file once (`O_RDONLY|O_CLOEXEC`), `pread` into a buffer sized at
   construction; returns parsed samples. Rates/deltas handle counter resets.
6. **Core**: register metrics in the `MetricRegistry`, sampled by the single `Sampler` tick;
   ring buffers preallocated. **No heap allocation per tick.**
7. **UI**: a `Panel` using existing `ChartWidget`/`Krell`/`Decal`; repaint only dirty rects.
8. **Verify**: `build-test` skill, then `perf-budget` skill (private memory, CPU, startup).
   Ask `perf-reviewer` to review the diff.
9. **Docs**: tick the item in `docs/roadmap.md`, add to `CHANGELOG.md` (Unreleased).
