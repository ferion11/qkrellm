// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/procfile.h"

namespace qk::core {

/// Load averages from /proc/loadavg ("load.1", "load.5", "load.15") and uptime in seconds
/// ("system.uptime", from CLOCK_BOOTTIME: what /proc/uptime reports, without reading a file).
/// The kernel recomputes the load every 5 s, hence the period; sampled once at construction.
class SystemMonitor final : public Monitor
{
public:
    static constexpr int kPeriodTicks = 5;

    explicit SystemMonitor(MetricRegistry& registry, const char* loadavgPath = "/proc/loadavg");

    void sample() override;
    [[nodiscard]] int periodTicks() const override { return kPeriodTicks; }

    /// /proc/loadavg was readable at construction.
    [[nodiscard]] bool hasLoad() const { return m_hasLoad; }
    [[nodiscard]] MetricHandle load1() const { return m_load1; }
    [[nodiscard]] MetricHandle load5() const { return m_load5; }
    [[nodiscard]] MetricHandle load15() const { return m_load15; }
    [[nodiscard]] MetricHandle uptime() const { return m_uptime; }

private:
    bool readLoad();
    void pushUptime();

    MetricRegistry* m_registry;
    sensors::ProcFile m_file;
    MetricHandle m_load1;
    MetricHandle m_load5;
    MetricHandle m_load15;
    MetricHandle m_uptime;
    bool m_hasLoad;
};

} // namespace qk::core
