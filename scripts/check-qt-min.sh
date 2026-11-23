#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Builds and tests against the MINIMUM supported Qt (ADR 0002), like the CI "Qt 6.5" job.
# Installs that Qt once with aqtinstall into /tmp/qkrellm/qt-min (needs network + python3).
#
#   scripts/check-qt-min.sh
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
version=6.5.3 # keep in sync with .github/workflows/ci.yml (min-qt job)
root="$out/qt-min"
prefix="$root/Qt/$version/gcc_64"
build="$out/build/qt-min"

if [[ ! -d "$prefix/lib/cmake/Qt6Widgets" ]]; then
    echo "Installing Qt $version into $root (one-time)..."
    python3 -m venv "$root/venv"
    "$root/venv/bin/pip" install -q aqtinstall
    (cd "$root" && "$root/venv/bin/aqt" install-qt linux desktop "$version" gcc_64 --outputdir "$root/Qt")
fi

cd "$repo"
cmake --preset dev -B "$build" -DCMAKE_PREFIX_PATH="$prefix" >/dev/null
cmake --build "$build" -- -k 0
QT_QPA_PLATFORM=offscreen ctest --test-dir "$build" --output-on-failure
echo "Qt $version: OK"
