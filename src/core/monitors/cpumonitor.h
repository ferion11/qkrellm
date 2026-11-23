// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/procfile.h"
#include "sensors/procstat.h"

#include <memory>
#include <vector>

namespace qk::core {

/// CPU usage from /proc/stat. Registers, for the aggregate line, "cpu.busy", "cpu.user",
/// "cpu.system" and, for every CPU present at construction, "cpu<N>.busy" (Fraction).
/// CPUs hot-plugged later are not added (they read as 0 if they go away).
class CpuMonitor final : public Monitor
{
public:
    explicit CpuMonitor(MetricRegistry& registry, const char* path = "/proc/stat");
    ~CpuMonitor() override;
    CpuMonitor(const CpuMonitor&) = delete;
    CpuMonitor& operator=(const CpuMonitor&) = delete;
    CpuMonitor(CpuMonitor&&) = delete;
    CpuMonitor& operator=(CpuMonitor&&) = delete;

    void sample() override;

    [[nodiscard]] bool isValid() const { return m_valid; }
    [[nodiscard]] MetricHandle busy() const { return m_busy; }
    [[nodiscard]] MetricHandle user() const { return m_user; }
    [[nodiscard]] MetricHandle system() const { return m_system; }
    [[nodiscard]] std::size_t cpuCount() const { return m_perCpu.size(); }
    [[nodiscard]] MetricHandle cpuBusy(std::size_t i) const { return m_perCpu.at(i).handle; }

private:
    bool readInto(sensors::ProcStat& out);

    struct PerCpu
    {
        std::size_t index = 0; // N of "cpuN"
        MetricHandle handle = 0;
    };

    MetricRegistry* m_registry;
    sensors::ProcFile m_file;
    std::unique_ptr<sensors::ProcStat> m_previous; // ~20 KiB each: on the heap, once
    std::unique_ptr<sensors::ProcStat> m_current;
    MetricHandle m_busy = 0;
    MetricHandle m_user = 0;
    MetricHandle m_system = 0;
    std::vector<PerCpu> m_perCpu;
    bool m_valid = false;
};

} // namespace qk::core
