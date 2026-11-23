// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// One "cpu" line of /proc/stat, in USER_HZ ticks.
/// Kernel docs: Documentation/filesystems/proc.rst, section "1.8 Miscellaneous kernel
/// statistics in /proc/stat". Columns appeared over time:
///   user nice system idle                (2.4)
///   + iowait irq softirq                 (2.6.0)
///   + steal                              (2.6.11)
///   + guest                              (2.6.24)
///   + guest_nice                         (2.6.33)
/// Columns missing on older kernels read as 0; `fields` says how many were present.
/// guest and guest_nice are already included in user and nice.
struct CpuTimes
{
    std::uint64_t user = 0;
    std::uint64_t nice = 0;
    std::uint64_t system = 0;
    std::uint64_t idle = 0;
    std::uint64_t iowait = 0;
    std::uint64_t irq = 0;
    std::uint64_t softirq = 0;
    std::uint64_t steal = 0;
    std::uint64_t guest = 0;
    std::uint64_t guestNice = 0;
    int fields = 0; ///< number of columns the kernel provided (4..10); 0 = line absent

    [[nodiscard]] bool present() const { return fields > 0; }
    /// All accounted time (guest columns excluded: they are part of user/nice).
    [[nodiscard]] std::uint64_t total() const
    {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    /// Time not spent idle or waiting for I/O.
    [[nodiscard]] std::uint64_t busy() const { return total() - idle - iowait; }
};

/// Parsed /proc/stat. Fixed-size so a reader can hold one without heap allocation.
struct ProcStat
{
    /// CPUs beyond this index are ignored and `cpusTruncated` is set.
    static constexpr int kMaxCpus = 256;

    CpuTimes total;                      ///< aggregate "cpu" line (always present)
    std::array<CpuTimes, kMaxCpus> cpus; ///< indexed by N of "cpuN"; offline CPUs are absent
    int cpuSlots = 0;                    ///< highest present index + 1
    bool cpusTruncated = false;

    std::optional<std::uint64_t> contextSwitches;  ///< "ctxt"
    std::optional<std::uint64_t> bootTime;         ///< "btime", seconds since the epoch
    std::optional<std::uint64_t> processesCreated; ///< "processes"
    std::optional<std::uint64_t> procsRunning;     ///< "procs_running"
    std::optional<std::uint64_t> procsBlocked;     ///< "procs_blocked"
};

/// Parses the text of /proc/stat. Tolerates missing and extra columns, unknown lines and
/// gaps in CPU numbering. Returns std::nullopt when the aggregate "cpu" line is missing or
/// malformed (fewer than 4 numbers, non-numeric or overflowing values).
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<ProcStat> parseProcStat(std::string_view text);

} // namespace qk::sensors
