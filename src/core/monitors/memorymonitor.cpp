// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/memorymonitor.h"

#include "sensors/meminfo.h"

#include <cstdint>

namespace qk::core {

namespace {
constexpr std::size_t kMeminfoCapacity = 8 * 1024; // /proc/meminfo is ~1.5 KiB

float fraction(std::uint64_t part, std::uint64_t whole)
{
    return whole == 0 ? 0.0F
                      : static_cast<float>(static_cast<double>(part) / static_cast<double>(whole));
}
} // namespace

MemoryMonitor::MemoryMonitor(MetricRegistry& registry, const char* path)
    : m_registry(&registry)
    , m_file(path, kMeminfoCapacity)
    , m_used(registry.add({.id = "mem.used", .label = "Mem", .unit = MetricUnit::Fraction}))
    , m_cache(registry.add({.id = "mem.cache", .label = "Cache", .unit = MetricUnit::Fraction}))
    , m_usedBytes(registry.add({.id = "mem.used.bytes", .label = "Mem", .unit = MetricUnit::Bytes}))
    , m_totalBytes(
          registry.add({.id = "mem.total.bytes", .label = "Mem total", .unit = MetricUnit::Bytes}))
    , m_swapUsed(registry.add({.id = "swap.used", .label = "Swap", .unit = MetricUnit::Fraction}))
    , m_swapUsedBytes(
          registry.add({.id = "swap.used.bytes", .label = "Swap", .unit = MetricUnit::Bytes}))
    , m_swapTotalBytes(registry.add(
          {.id = "swap.total.bytes", .label = "Swap total", .unit = MetricUnit::Bytes}))
    , m_valid(read())
{
    m_hasSwap = m_valid && m_registry->history(m_swapTotalBytes).latest() > 0.0F;
}

void MemoryMonitor::sample()
{
    (void)read(); // on failure the charts simply get no new point
}

bool MemoryMonitor::read()
{
    const auto text = m_file.read();
    if (!text) {
        return false;
    }
    const auto info = sensors::parseMemInfo(*text);
    if (!info) {
        return false;
    }
    const auto usage = sensors::computeMemoryUsage(*info);
    m_registry->push(m_used, fraction(usage.used, usage.total));
    m_registry->push(m_cache, fraction(usage.cache, usage.total));
    m_registry->push(m_usedBytes, static_cast<float>(usage.used));
    m_registry->push(m_totalBytes, static_cast<float>(usage.total));
    m_registry->push(m_swapUsed, fraction(usage.swapUsed, usage.swapTotal));
    m_registry->push(m_swapUsedBytes, static_cast<float>(usage.swapUsed));
    m_registry->push(m_swapTotalBytes, static_cast<float>(usage.swapTotal));
    return true;
}

} // namespace qk::core
