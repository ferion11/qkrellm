#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Static analysis for qkrellm. Uses the out-of-tree dev build's compile database.
#   scripts/lint.sh            # format check + clang-tidy + clazy + cppcheck (tools that exist)
#   scripts/lint.sh format     # only clang-format check
#   scripts/lint.sh fix        # apply clang-format in place
set -euo pipefail

repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
build="$out/build/dev"
db="$out/lint-db"   # sanitised compile database for clang-based tools
mode=${1:-all}

cd "$repo"
mapfile -t sources < <(git ls-files '*.c' '*.cpp' '*.h' '*.hpp')
# tests/fuzz is compiled only by the 'fuzz' preset, so it is absent from the dev database.
mapfile -t tus < <(git ls-files 'src/*.cpp' 'tests/*.cpp' 'tools/*.cpp' 'plugins/*.c' 'tests/*.c' ':!tests/fuzz/*')

have() { command -v "$1" >/dev/null 2>&1; }
status=0

run_format() {
    echo "== clang-format"
    if [[ "$1" == fix ]]; then
        clang-format -i "${sources[@]}"
    else
        clang-format --dry-run --Werror "${sources[@]}" || status=1
    fi
}

ensure_db() {
    if [[ ! -f "$build/compile_commands.json" ]]; then
        echo "No compile database; configuring dev preset..."
        cmake --preset dev >/dev/null
        cmake --build --preset dev >/dev/null  # generates moc/autogen headers
    fi
}

case "$mode" in
    format) run_format check; exit $status ;;
    fix)    run_format fix; exit 0 ;;
    all)    ;;
    *) echo "usage: $0 [all|format|fix]" >&2; exit 2 ;;
esac

run_format check
ensure_db

# Qt adds GCC-only flags (e.g. -mno-direct-extern-access) that clang-based tools reject.
mkdir -p "$db"
python3 -I - "$build/compile_commands.json" "$db/compile_commands.json" <<'PY'
import json, sys
gcc_only = {"-mno-direct-extern-access", "-fno-direct-access-external-data"}
entries = json.load(open(sys.argv[1]))
for e in entries:
    if "arguments" in e:
        e["arguments"] = [a for a in e["arguments"] if a not in gcc_only]
    else:
        e["command"] = " ".join(a for a in e["command"].split(" ") if a not in gcc_only)
json.dump(entries, open(sys.argv[2], "w"), indent=1)
PY

if have clang-tidy; then
    echo "== clang-tidy"
    # Capture first: with pipefail, `tool | grep && status=1` would miss failures whenever
    # the tool itself exits non-zero.
    findings=$(clang-tidy -p "$db" --quiet "${tus[@]}" 2>/dev/null | grep -E 'warning:|error:' || true)
    [[ -z "$findings" ]] || { echo "$findings"; status=1; }
fi

if have clazy-standalone; then
    echo "== clazy (level1)"
    mapfile -t qt_tus < <(git ls-files 'src/*.cpp')
    findings=$(clazy-standalone -p "$db" -checks=level1 --ignore-dirs="^/tmp/" \
        --header-filter="/(src|include)/" "${qt_tus[@]}" 2>&1 | grep -E 'warning:|error:' || true)
    [[ -z "$findings" ]] || { echo "$findings"; status=1; }
fi

if have cppcheck; then
    echo "== cppcheck"
    mkdir -p "$out/cppcheck"
    cppcheck --project="$db/compile_commands.json" --library=qt \
        --enable=warning,performance,portability --inline-suppr \
        --suppress=missingIncludeSystem --suppress='*:/tmp/*' --suppress=preprocessorErrorDirective \
        --error-exitcode=1 --quiet --cppcheck-build-dir="$out/cppcheck" || status=1
fi

(( status == 0 )) && echo "lint: OK" || echo "lint: FAILED"
exit $status
