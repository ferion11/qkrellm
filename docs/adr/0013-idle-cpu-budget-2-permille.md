# 0013 — Idle CPU budget: 2 ‰ with the default monitor set

- **Status:** Accepted
- **Date:** 2026-11-23
- **Deciders:** maintainer + agent session
- **Amends:** [0006](0006-resource-budget-is-a-requirement.md) (its idle-CPU limit only; memory and startup
  limits are unchanged)

## Context

ADR 0006 set idle CPU ≤ 1 ‰ (0.1 % of one core) when the app showed one CPU panel. With the
whole M2 set (CPU, memory, disk, network ×2, hwmon, battery) on the reference laptop
(20 CPUs, Linux 6.18, X11, CPU idle at low clock) it measured 1.2–1.4 ‰ after these
optimisations: disks read from /sys/block/<d>/stat instead of /proc/diskstats, slow hwmon
inputs read every 10–30 s (timed on first read), battery every 10 s, headers repainted only
when their text changes, charts repainted only when their pixels would change and scrolled
(one new column) otherwise.

Instrumented per tick (thread CPU time, cold caches): reading and parsing the kernel files
≈ 0.6 ms — /proc/stat alone ≈ 0.14–0.2 ms on 20 CPUs, inherent to the kernel generating
it — and Qt's repaint/event machinery ≈ 0.4–0.6 ms.

The previous measurement counted /proc/<pid>/stat clock ticks (10 ms each: 1 ‰ over 30 s is
3 ticks) and rounded down, which had hidden values up to 1.9 ‰ since the network monitor.
For reference, GKrellM 2 on the same machine measured 6.1 ‰ CPU and 6.3 MiB private memory
by the same method; qkrellm 1.2–1.4 ‰ and 2.8 MiB.

## Decision

- Idle CPU budget: **≤ 2 ‰ of one core** with the default monitor set at 1 Hz
  (`QK_BUDGET_CPU_PERMILLE=2` in `benchmarks/budget.env`).
- CPU is measured from `/proc/<pid>/schedstat` (nanoseconds) with two decimals, never from
  clock ticks.
- Optimisations listed above are kept; a new monitor must not read anything per tick that
  costs more than ~0.1 ms of CPU without lowering its period.

## Consequences

- About 0.6–0.8 ‰ of headroom for the remaining M2 monitors (filesystems, clock/uptime) and
  themes; each must be measured with `benchmarks/measure.sh`.
- Machines with many CPUs pay more for /proc/stat; the budget is checked on the reference
  laptop and in CI (where only memory and CPU are strict).
- Revisit if the default monitor set grows, or if sampling at 2 s becomes the default.
