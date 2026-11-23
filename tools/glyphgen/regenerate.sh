#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Regenerates the committed glyph atlases in src/ui/fonts/ (ADR 0011).
# Output depends on the FreeType/Qt version, so atlases are committed, not built.
#
#   tools/glyphgen/regenerate.sh [font.ttf]     (default: system DejaVu Sans Condensed)
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
out=${QKRELLM_OUT:-/tmp/qkrellm}
font=${1:-/usr/share/fonts/dejavu/DejaVuSansCondensed.ttf}
size=${QK_ATLAS_PIXEL_SIZE:-9} # fits the 70 px default panel (Theme::panelWidth)

cd "$repo"
cmake --preset dev >/dev/null
cmake --build --preset dev --target qkrellm-glyphgen
gen="$out/build/dev/tools/glyphgen/qkrellm-glyphgen"
export QT_QPA_PLATFORM=offscreen
"$gen" "$font" "$size" 1 "$repo/src/ui/fonts/default"
"$gen" "$font" "$size" 2 "$repo/src/ui/fonts/default@2x"
echo "Review the PNGs, run the tests, then commit src/ui/fonts/."
