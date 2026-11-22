#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Usage:
#   check-qt-modules.sh <binary> <AllowedModule>...   fail on any Qt6 lib not in the allowlist
#   check-qt-modules.sh <binary> --forbid-cxx         fail on ANY Qt6 lib or libstdc++ (C plugins)
# Reads DT_NEEDED entries with readelf (ADR 0002, docs/resource-budget.md).
set -euo pipefail

binary=${1:?binary path required}
shift

needed=$(readelf -d "$binary" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p')

if [[ "${1:-}" == "--forbid-cxx" ]]; then
    if bad=$(grep -E '^(libQt6|libstdc\+\+|libc\+\+)' <<<"$needed"); then
        echo "FORBIDDEN in pure-C artifact $binary:" >&2
        echo "$bad" >&2
        exit 1
    fi
    echo "ok: no Qt / C++ runtime in $(basename "$binary")"
    exit 0
fi

allowed=("$@")
status=0
while read -r lib; do
    [[ "$lib" =~ ^libQt6([A-Za-z]+)\.so ]] || continue
    module=${BASH_REMATCH[1]}
    if [[ " ${allowed[*]} " == *" $module "* ]]; then
        echo "ok:        Qt6$module"
    else
        echo "FORBIDDEN: Qt6$module (allowed: ${allowed[*]}) — needs an ADR, see docs/adr/0002-*.md" >&2
        status=1
    fi
done <<<"$needed"
exit $status
