# 0006 — The resource budget is a tested requirement

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

"Light on resources" is the product's reason to exist, so it must be measurable and
regressions must fail a check, not be noticed months later.

The initial plan budgeted RSS ≤ 20 MB. Measuring the M0 hello window (Qt 6.11.3, release
build) showed RSS ≈ 53 MB of which ≈ 41 MB are **shared file-backed pages** (Qt libraries,
fontconfig caches, locale archive) that every Qt application maps; private memory was
≈ 11.5 MB (offscreen) / 12.0 MB (xcb), PSS ≈ 23 MB. RSS is therefore dominated by
things our code cannot change.

## Decision

- Budget metrics (defaults in `benchmarks/budget.env`):
  - **private memory** (`Private_Dirty`, smaps_rollup) at idle ≤ **16 MiB**;
  - idle CPU ≤ **0.1 %** of one core at the default sampling rate;
  - startup (process start → first paint, best of 5) ≤ **150 ms**.
- RSS and PSS are recorded for information only.
- `benchmarks/measure.sh --check` enforces the budget; CI runs it (startup limit relaxed
  in CI because shared runners are noisy). Results accumulate in
  `/tmp/qkrellm/bench/results.tsv`.
- Changing a limit requires a new ADR with measurements.

## Consequences

- Every feature PR reports `measure.sh` output (PR template).
- M0 already exceeds the startup target on xcb (≈ 158–170 ms) because of the first text
  draw; this is tracked in `docs/roadmap.md` as an M1 investigation.
