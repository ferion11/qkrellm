// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/cpumonitor.h"

#include "sensors/cpuusage.h"

#include <string>
#include <utility>

namespace qk::core {

CpuMonitor::CpuMonitor(MetricRegistry& registry, const char* path)
    : m_registry(&registry)
    , m_file(path)
    , m_previous(std::make_unique<sensors::ProcStat>())
    , m_current(std::make_unique<sensors::ProcStat>())
    , m_busy(registry.add({.id = "cpu.busy", .label = "CPU", .unit = MetricUnit::Fraction}))
    , m_user(registry.add({.id = "cpu.user", .label = "CPU user", .unit = MetricUnit::Fraction}))
    , m_system(registry.add({.id = "cpu.system", .label = "CPU sys", .unit = MetricUnit::Fraction}))
    // The first read is the baseline: the first sample() can already compute usage.
    , m_valid(readInto(*m_previous))
{
    if (!m_valid) {
        return;
    }
    for (int i = 0; i < m_previous->cpuSlots; ++i) {
        const auto index = static_cast<std::size_t>(i);
        if (m_previous->cpus.at(index).present()) {
            const std::string n = std::to_string(i);
            const MetricHandle handle = registry.add(
                {.id = "cpu" + n + ".busy", .label = "CPU" + n, .unit = MetricUnit::Fraction});
            m_perCpu.push_back({.index = index, .handle = handle});
        }
    }
}

CpuMonitor::~CpuMonitor() = default;

bool CpuMonitor::readInto(sensors::ProcStat& out)
{
    const auto text = m_file.read();
    if (!text) {
        return false;
    }
    auto parsed = sensors::parseProcStat(*text);
    if (!parsed) {
        return false;
    }
    out = *parsed;
    return true;
}

void CpuMonitor::sample()
{
    if (!readInto(*m_current)) {
        return; // keep the previous baseline; charts simply get no new point
    }
    const auto total = sensors::computeUsage(m_previous->total, m_current->total);
    m_registry->push(m_busy, total.busy);
    m_registry->push(m_user, total.user);
    m_registry->push(m_system, total.system);
    for (const PerCpu& cpu : m_perCpu) {
        const auto usage =
            sensors::computeUsage(m_previous->cpus.at(cpu.index), m_current->cpus.at(cpu.index));
        m_registry->push(cpu.handle, usage.busy);
    }
    std::swap(m_previous, m_current);
}

} // namespace qk::core
