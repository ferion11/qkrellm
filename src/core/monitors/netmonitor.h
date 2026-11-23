// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/netdev.h"
#include "sensors/procfile.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace qk::core {

/// Network throughput from /proc/net/dev. Interfaces are chosen once, at construction:
/// every interface except "lo" that is backed by a device (`<sysClassNet>/<name>/device`
/// exists, i.e. not a bridge, veth, tun or other virtual one); if there is none, every
/// interface except "lo". For each it registers "net.<name>.rx", "net.<name>.tx" and
/// "net.<name>.total" in bytes per second (MetricUnit::BytesPerSecond).
/// Interfaces that appear later are not added; one that disappears reads 0 until it is back.
class NetMonitor final : public Monitor
{
public:
    using Clock = std::chrono::steady_clock;

    explicit NetMonitor(MetricRegistry& registry, const char* path = "/proc/net/dev",
                        const char* sysClassNet = "/sys/class/net");
    ~NetMonitor() override;
    NetMonitor(const NetMonitor&) = delete;
    NetMonitor& operator=(const NetMonitor&) = delete;
    NetMonitor(NetMonitor&&) = delete;
    NetMonitor& operator=(NetMonitor&&) = delete;

    void sample() override;
    /// Same as sample() with an explicit timestamp (tests).
    void sampleAt(Clock::time_point now);

    struct Interface
    {
        std::string name;
        MetricHandle rx = 0;
        MetricHandle tx = 0;
        MetricHandle total = 0;
    };

    [[nodiscard]] bool isValid() const { return m_valid; }
    [[nodiscard]] const std::vector<Interface>& interfaces() const { return m_interfaces; }

private:
    struct Counters
    {
        std::uint64_t rx = 0;
        std::uint64_t tx = 0;
        bool present = false;
    };

    bool readInto(sensors::NetDev& out);

    MetricRegistry* m_registry;
    sensors::ProcFile m_file;
    std::unique_ptr<sensors::NetDev> m_current; // ~3 KiB, allocated once
    std::vector<Interface> m_interfaces;
    std::vector<Counters> m_previous; // parallel to m_interfaces
    Clock::time_point m_previousTime;
    bool m_valid = false;
};

} // namespace qk::core
