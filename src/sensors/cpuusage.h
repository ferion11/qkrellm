// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sensors/procstat.h"

namespace qk::sensors {

/// CPU usage between two samples, as fractions of the elapsed accounted time (0..1).
struct CpuUsage
{
    float busy = 0.0F;   ///< everything but idle and iowait
    float user = 0.0F;   ///< user + nice
    float system = 0.0F; ///< system + irq + softirq
    float iowait = 0.0F;
    float steal = 0.0F;
};

/// Computes usage from `previous` to `current`. Counters that went backwards (CPU
/// hotplug, iowait accounting quirks, a different CPU at the same index) contribute 0
/// instead of wrapping; no elapsed time yields all zeros. Pure, allocation-free.
[[nodiscard]] CpuUsage computeUsage(const CpuTimes& previous, const CpuTimes& current);

} // namespace qk::sensors
