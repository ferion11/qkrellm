#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Snapshot the sysfs parts qkrellm reads, for fixtures:
#  - /sys/class/hwmon: "name", temp*/fan* "_input" and "_label", and each "device" link
#    (re-created as a relative link to an empty directory);
#  - /sys/class/power_supply/*/uevent, with SERIAL_NUMBER blanked.
# Writes /tmp/qkrellm/scratch/<tag>/sys/ and a tarball next to it; review, then extract into
# tests/fixtures/sys/<tag>/ (tar xf <tarball> -C tests/fixtures/sys/<tag>).
#
#   tests/fixtures/capture-sys.sh <tag>
set -euo pipefail
tag=${1:?usage: capture-sys.sh <tag>}
scratch=${QKRELLM_OUT:-/tmp/qkrellm}/scratch/$tag
out=$scratch/sys
rm -rf "$out" && mkdir -p "$out/class/hwmon" "$out/devices"
for h in /sys/class/hwmon/hwmon*; do
    [[ -e "$h" ]] || continue
    n=$(basename "$h")
    d=$out/class/hwmon/$n
    mkdir -p "$d"
    cat "$h/name" >"$d/name" 2>/dev/null || true
    for f in "$h"/temp*_input "$h"/temp*_label "$h"/fan*_input "$h"/fan*_label; do
        [[ -e "$f" ]] && { cat "$f" >"$d/$(basename "$f")" 2>/dev/null || rm -f "$d/$(basename "$f")"; }
    done
    if dev=$(readlink -f "$h/device" 2>/dev/null) && [[ -n "$dev" ]]; then
        dev=$(basename "$dev")
        mkdir -p "$out/devices/$dev" && touch "$out/devices/$dev/.keep"
        ln -s "../../../devices/$dev" "$d/device"
    fi
done
for p in /sys/class/power_supply/*; do
    [[ -r "$p/uevent" ]] || continue
    mkdir -p "$out/class/power_supply/$(basename "$p")"
    sed 's/^\(POWER_SUPPLY_SERIAL_NUMBER=\).*/\1/' "$p/uevent" \
        >"$out/class/power_supply/$(basename "$p")/uevent"
done
uname -srm >"$out/UNAME"
tar -C "$out" -cf "$scratch/sys-$tag.tar" .
echo "Captured $(find "$out/class/hwmon" -name '*_input' | wc -l) hwmon inputs and" \
    "$(find "$out/class" -name uevent | wc -l) power supplies -> $scratch/sys-$tag.tar"
