// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/systemmonitor.h"

#include "sensors/loadavg.h"

#include <ctime>

namespace qk::core {

namespace {
constexpr std::size_t kLoadavgCapacity = 256;
} // namespace

SystemMonitor::SystemMonitor(MetricRegistry& registry, const char* loadavgPath)
    : m_registry(&registry)
    , m_file(loadavgPath, kLoadavgCapacity)
    , m_load1(registry.add({.id = "load.1", .label = "Load", .unit = MetricUnit::Count}))
    , m_load5(registry.add({.id = "load.5", .label = "Load 5", .unit = MetricUnit::Count}))
    , m_load15(registry.add({.id = "load.15", .label = "Load 15", .unit = MetricUnit::Count}))
    , m_uptime(registry.add({.id = "system.uptime", .label = "Up", .unit = MetricUnit::Count}))
    , m_hasLoad(readLoad())
{
    pushUptime();
}

void SystemMonitor::sample()
{
    (void)readLoad(); // on failure the load simply gets no new point
    pushUptime();
}

bool SystemMonitor::readLoad()
{
    const auto text = m_file.read();
    const auto load = text ? sensors::parseLoadAvg(*text) : std::nullopt;
    if (!load) {
        return false;
    }
    m_registry->push(m_load1, static_cast<float>(load->one));
    m_registry->push(m_load5, static_cast<float>(load->five));
    m_registry->push(m_load15, static_cast<float>(load->fifteen));
    return true;
}

void SystemMonitor::pushUptime()
{
    timespec now{};
    if (::clock_gettime(CLOCK_BOOTTIME, &now) == 0) {
        m_registry->push(m_uptime, static_cast<float>(now.tv_sec));
    }
}

} // namespace qk::core
