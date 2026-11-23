---
name: perf-budget
description: Measure qkrellm startup time, private memory, PSS/RSS and idle CPU against the resource budget, and profile memory with heaptrack/valgrind. Use after any change to sampling, painting, startup, dependencies or data structures, and whenever a PR must report budget numbers.
---

# Check the resource budget

Budget (ADR 0006, `docs/resource-budget.md`, defaults in `benchmarks/budget.env`):
private memory ≤ 16 MiB · idle CPU ≤ 0.2 % (2 ‰, ADR 0013) · startup ≤ 150 ms · Qt Core/Gui/Widgets only.

## Measure

```sh
cmake --preset release && cmake --build --preset release
benchmarks/measure.sh --check                       # offscreen (CI-comparable)
QT_QPA_PLATFORM=xcb benchmarks/measure.sh --check   # real X11 (needs DISPLAY)
ctest --preset release -R budget                     # linked Qt modules
column -t /tmp/qkrellm/bench/results.tsv | tail     # history
```
Compare against the previous row for the same platform and the baseline table in
`docs/resource-budget.md`. Report the `measure.sh` line in the PR description.

## Investigate a regression

- Heap: `heaptrack -o /tmp/qkrellm/bench/heap /tmp/qkrellm/build/release/src/app/qkrellm`
  (close after ~10 s) then `heaptrack_print /tmp/qkrellm/bench/heap.*.zst | head -60`.
  For hot-path allocations look at "most calls to allocation functions".
- Massif: `valgrind --tool=massif --massif-out-file=/tmp/qkrellm/bench/massif.out <bin> --exit-after-first-paint`
- Mappings: `awk '/^[0-9a-f]+-/{f=$6} /^Private_Dirty:/{r[f]+=$2} END{for(k in r) print r[k], k}' /proc/<pid>/smaps | sort -rn | head`
- Startup: anything that constructs a `QFont`/`QFontMetrics` or calls `drawText` before
  the first paint populates Qt's font database (≈ 120 ms and ≈ 10 MiB with many installed
  fonts — ADR 0011); `tst_bitmaptext` guards the panel path. Time phases with
  `QElapsedTimer` printouts in a scratch build (or `benchmarks/text-cost/`), not in
  committed code.

Changing a limit requires a new ADR with before/after numbers.
