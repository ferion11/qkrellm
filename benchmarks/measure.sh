#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Measures startup, idle private memory (plus RSS/PSS for information) and idle CPU of a qkrellm binary and compares with the budget.
#
#   benchmarks/measure.sh [--check] [--seconds N] [--binary PATH]
#
# Defaults: release build at /tmp/qkrellm/build/release/src/app/qkrellm, 10 s idle window,
# QT_QPA_PLATFORM=offscreen unless already set (xcb adds ~0.5 MiB private memory).
# Results are appended to /tmp/qkrellm/bench/results.tsv. With --check, exit 1 on any overrun.
set -euo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
binary="$out/build/release/src/app/qkrellm"
seconds=10
check=0
while (( $# )); do
    case "$1" in
        --check) check=1 ;;
        --seconds) seconds=$2; shift ;;
        --binary) binary=$2; shift ;;
        *) echo "unknown option $1" >&2; exit 2 ;;
    esac
    shift
done

# shellcheck source=budget.env
source "$repo/benchmarks/budget.env"
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-offscreen}
[[ -x "$binary" ]] || { echo "binary not found: $binary (build the 'release' preset)" >&2; exit 2; }
mkdir -p "$out/bench"

# 1) Startup: best of 5 runs.
startup=999999
for _ in 1 2 3 4 5; do
    ms=$("$binary" --exit-after-first-paint | sed -n 's/^first-paint-ms=//p')
    (( ms >= 0 && ms < startup )) && startup=$ms
done

# 2) Idle memory + CPU over the window.
"$binary" & pid=$!
trap 'kill $pid 2>/dev/null || true' EXIT
sleep 2  # settle
ticks() { awk '{print $14 + $15}' "/proc/$pid/stat"; }
t0=$(ticks)
sleep "$seconds"
t1=$(ticks)
rss=$(awk '/^VmRSS:/ {print $2}' "/proc/$pid/status")
private=$(awk '/^Private_Dirty:/ {print $2}' "/proc/$pid/smaps_rollup")
pss=$(awk '/^Pss:/ {print $2}' "/proc/$pid/smaps_rollup")
kill "$pid"; wait "$pid" 2>/dev/null || true
trap - EXIT

hz=$(getconf CLK_TCK)
cpu_permille=$(( (t1 - t0) * 1000 / (hz * seconds) ))

printf "startup_ms=%s private_kb=%s pss_kb=%s rss_kb=%s cpu_permille=%s platform=%s\n" \
    "$startup" "$private" "$pss" "$rss" "$cpu_permille" "$QT_QPA_PLATFORM"
[[ -f "$out/bench/results.tsv" ]] ||
    printf "date\tcommit\tstartup_ms\tprivate_kb\tpss_kb\trss_kb\tcpu_permille\tplatform\n" >"$out/bench/results.tsv"
printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" "$(date -Is)" \
    "$(git -C "$repo" rev-parse --short HEAD 2>/dev/null || echo -)" \
    "$startup" "$private" "$pss" "$rss" "$cpu_permille" "$QT_QPA_PLATFORM" >>"$out/bench/results.tsv"

status=0
if (( check )); then
    (( startup <= QK_BUDGET_STARTUP_MS )) || { echo "OVER BUDGET: startup ${startup} ms > ${QK_BUDGET_STARTUP_MS}" >&2; status=1; }
    (( private <= QK_BUDGET_PRIVATE_KB )) || { echo "OVER BUDGET: private ${private} kB > ${QK_BUDGET_PRIVATE_KB}" >&2; status=1; }
    (( cpu_permille <= QK_BUDGET_CPU_PERMILLE )) || { echo "OVER BUDGET: cpu ${cpu_permille}‰ > ${QK_BUDGET_CPU_PERMILLE}" >&2; status=1; }
    (( status == 0 )) && echo "budget: OK"
fi
exit $status
