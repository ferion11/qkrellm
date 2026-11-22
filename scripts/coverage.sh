#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Build with gcov, run tests, write HTML+XML reports to /tmp/qkrellm/coverage.
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
cd "$repo"
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage
mkdir -p "$out/coverage"
gcovr --root "$repo" --object-directory "$out/build/coverage" \
    --filter 'src/' --filter 'include/' \
    --html-details "$out/coverage/index.html" --xml "$out/coverage/coverage.xml" --print-summary
echo "Report: $out/coverage/index.html"
