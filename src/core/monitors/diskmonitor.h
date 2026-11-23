// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/diskstats.h"
#include "sensors/procfile.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace qk::core {

/// Disk throughput from /proc/diskstats. Disks are chosen once, at construction: whole
/// disks backed by a device (`<sysBlock>/<name>/device` exists — excludes partitions, loop,
/// ram, zram, dm and md devices); if there is none, every whole disk in `<sysBlock>` except
/// loop and ram ones. Registers "disk.<name>.read" / ".write" per disk and the aggregates
/// "disk.read", "disk.write", "disk.total", in bytes per second.
/// Disks that appear later are not added; one that disappears reads 0 until it is back.
class DiskMonitor final : public Monitor
{
public:
    using Clock = std::chrono::steady_clock;

    explicit DiskMonitor(MetricRegistry& registry, const char* path = "/proc/diskstats",
                         const char* sysBlock = "/sys/block");
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

    [[nodiscard]] bool isValid() const { return m_valid; }
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

    bool readInto(sensors::DiskStats& out);

    MetricRegistry* m_registry;
    sensors::ProcFile m_file;
    std::unique_ptr<sensors::DiskStats> m_current; // ~14 KiB, allocated once
    MetricHandle m_read;
    MetricHandle m_write;
    MetricHandle m_total;
    std::vector<Disk> m_disks;
    std::vector<Counters> m_previous; // parallel to m_disks
    Clock::time_point m_previousTime;
    bool m_valid = false;
};

} // namespace qk::core
