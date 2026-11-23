// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/cpuusage.h"

namespace qk::sensors {

namespace {

std::uint64_t delta(std::uint64_t previous, std::uint64_t current)
{
    return current > previous ? current - previous : 0;
}

} // namespace

CpuUsage computeUsage(const CpuTimes& previous, const CpuTimes& current)
{
    const std::uint64_t user =
        delta(previous.user, current.user) + delta(previous.nice, current.nice);
    const std::uint64_t system = delta(previous.system, current.system) +
                                 delta(previous.irq, current.irq) +
                                 delta(previous.softirq, current.softirq);
    const std::uint64_t idle = delta(previous.idle, current.idle);
    const std::uint64_t iowait = delta(previous.iowait, current.iowait);
    const std::uint64_t steal = delta(previous.steal, current.steal);

    // Sum of clamped per-column deltas, so fractions always add up to at most 1.
    const std::uint64_t total = user + system + idle + iowait + steal;
    if (total == 0) {
        return {};
    }
    const auto fraction = [total](std::uint64_t part) {
        return static_cast<float>(static_cast<double>(part) / static_cast<double>(total));
    };
    CpuUsage usage;
    usage.user = fraction(user);
    usage.system = fraction(system);
    usage.iowait = fraction(iowait);
    usage.steal = fraction(steal);
    usage.busy = fraction(user + system + steal);
    return usage;
}

} // namespace qk::sensors
