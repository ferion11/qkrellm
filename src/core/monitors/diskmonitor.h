// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/procfile.h"

#include <chrono>
#include <string>
#include <vector>

namespace qk::core {

/// Disk throughput from /sys/block/<name>/stat (one small file per disk: measured ~25 µs per
/// tick for two disks, against ~100 µs for /proc/diskstats, which also lists every ram, loop
/// and partition). Disks are chosen once, at construction: entries of `sysBlock` backed by a
/// device (`<name>/device` exists — excludes loop, ram, zram, dm and md devices); if there is
/// none, every entry except loop and ram ones. Partitions are not listed in /sys/block.
/// Registers "disk.<name>.read" / ".write" per disk and the aggregates "disk.read",
/// "disk.write", "disk.total", in bytes per second. Disks that appear later are not added.
class DiskMonitor final : public Monitor
{
public:
    using Clock = std::chrono::steady_clock;

    explicit DiskMonitor(MetricRegistry& registry, const std::string& sysBlock = "/sys/block");
    ~DiskMonitor() override;
    DiskMonitor(const DiskMonitor&) = delete;
    DiskMonitor& operator=(const DiskMonitor&) = delete;
    DiskMonitor(DiskMonitor&&) = delete;
    DiskMonitor& operator=(DiskMonitor&&) = delete;

    void sample() override;
    /// Same as sample() with an explicit timestamp (tests).
    void sampleAt(Clock::time_point now);

    struct Disk
    {
        std::string name;
        MetricHandle read = 0;
        MetricHandle write = 0;
    };

    [[nodiscard]] const std::vector<Disk>& disks() const { return m_disks; }
    [[nodiscard]] MetricHandle read() const { return m_read; }
    [[nodiscard]] MetricHandle write() const { return m_write; }
    [[nodiscard]] MetricHandle total() const { return m_total; }

private:
    struct Counters
    {
        std::uint64_t sectorsRead = 0;
        std::uint64_t sectorsWritten = 0;
        bool present = false;
    };

    MetricRegistry* m_registry;
    std::vector<sensors::ProcFile> m_files; // parallel to m_disks
    MetricHandle m_read;
    MetricHandle m_write;
    MetricHandle m_total;
    std::vector<Disk> m_disks;
    std::vector<Counters> m_previous; // parallel to m_disks
    Clock::time_point m_previousTime;
};

} // namespace qk::core
