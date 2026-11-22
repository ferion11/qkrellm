#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Snapshot kernel files for parser fixtures. Writes to /tmp/qkrellm/scratch/<tag>/ first so
# you can REVIEW AND SCRUB them (hostnames, MACs, serials, mount paths) before copying the
# ones you need into tests/fixtures/proc/<tag>/.
#
#   tests/fixtures/capture.sh <tag> [path ...]
#   tag example: "linux-6.18-x86_64-desktop"
set -euo pipefail
tag=${1:?usage: capture.sh <tag> [path ...]}
shift
out=${QKRELLM_OUT:-/tmp/qkrellm}/scratch/$tag
paths=("$@")
if (( ${#paths[@]} == 0 )); then
    paths=(/proc/stat /proc/meminfo /proc/net/dev /proc/diskstats /proc/uptime /proc/loadavg
           /proc/mounts)
fi
mkdir -p "$out"
for p in "${paths[@]}"; do
    dest="$out${p}"
    mkdir -p "$(dirname "$dest")"
    cat "$p" >"$dest"   # cat, not cp: procfs files report size 0
    echo "captured $p"
done
uname -srm >"$out/UNAME"
echo
echo "Review $out, scrub personal data, then copy the needed files to tests/fixtures/proc/$tag/"
