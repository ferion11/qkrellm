#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds the 'fuzz' preset and runs every parser fuzzer for N seconds each.
#
#   scripts/fuzz.sh [seconds-per-fuzzer]      (default 60)
#
# Corpora persist in /tmp/qkrellm/fuzz/corpus/<fuzzer> (seeded from the repo every run);
# crashes from the latest run land in /tmp/qkrellm/fuzz/crashes/<fuzzer>/ (reproduce with
# <fuzzer binary> <crash file>). Exit status is non-zero on a crash.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
seconds=${1:-60}
bin="$out/build/fuzz/tests/fuzz"

cd "$repo"
cmake --preset fuzz >/dev/null
cmake --build --preset fuzz --target fuzz_procstat fuzz_meminfo fuzz_textatlas

seed() { # <fuzzer> <files...>
    local corpus="$out/fuzz/corpus/$1"
    shift
    mkdir -p "$corpus"
    for f in "$@"; do cp -f "$f" "$corpus/seed-$(basename "$(dirname "$f")")-$(basename "$f")"; done
}
seed fuzz_procstat tests/fixtures/proc/*/proc/stat*
seed fuzz_meminfo tests/fixtures/proc/*/proc/meminfo*
seed fuzz_textatlas src/ui/fonts/*.qkatlas

status=0
for fuzzer in fuzz_procstat fuzz_meminfo fuzz_textatlas; do
    crashes="$out/fuzz/crashes/$fuzzer"
    rm -rf "$crashes" && mkdir -p "$crashes" # only report crashes from this run
    echo "== $fuzzer (${seconds}s)"
    QT_QPA_PLATFORM=offscreen "$bin/$fuzzer" -max_total_time="$seconds" -print_final_stats=1 \
        -artifact_prefix="$crashes/" "$out/fuzz/corpus/$fuzzer" 2>&1 |
        grep -E "^(stat::number_of_executed_units|stat::peak_rss|==[0-9]+==ERROR|SUMMARY)|Done [0-9]+ runs" ||
        true
    if compgen -G "$crashes/crash-*" >/dev/null || compgen -G "$crashes/leak-*" >/dev/null; then
        echo "CRASH in $fuzzer: see $crashes" >&2
        status=1
    fi
done
exit $status
