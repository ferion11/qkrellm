// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/procfile.h"

namespace qk::core {

/// Memory and swap from /proc/meminfo (see sensors::computeMemoryUsage for definitions).
/// Registers "mem.used" and "mem.cache" (Fraction of MemTotal, stackable), "swap.used"
/// (Fraction of SwapTotal, 0 without swap) and the byte values "mem.used.bytes",
/// "mem.total.bytes", "swap.used.bytes", "swap.total.bytes".
/// Memory is a level, not a rate: the constructor already pushes the first sample.
class MemoryMonitor final : public Monitor
{
public:
    explicit MemoryMonitor(MetricRegistry& registry, const char* path = "/proc/meminfo");

    void sample() override;

    [[nodiscard]] bool isValid() const { return m_valid; }
    /// Swap was configured at construction (swap enabled later shows up in the metrics,
    /// but the app decides at startup whether to show a swap panel).
    [[nodiscard]] bool hasSwap() const { return m_hasSwap; }

    [[nodiscard]] MetricHandle used() const { return m_used; }
    [[nodiscard]] MetricHandle cache() const { return m_cache; }
    [[nodiscard]] MetricHandle usedBytes() const { return m_usedBytes; }
    [[nodiscard]] MetricHandle totalBytes() const { return m_totalBytes; }
    [[nodiscard]] MetricHandle swapUsed() const { return m_swapUsed; }
    [[nodiscard]] MetricHandle swapUsedBytes() const { return m_swapUsedBytes; }
    [[nodiscard]] MetricHandle swapTotalBytes() const { return m_swapTotalBytes; }

private:
    /// Reads, parses and pushes one sample. Returns false (pushing nothing) on failure.
    bool read();

    MetricRegistry* m_registry;
    sensors::ProcFile m_file;
    MetricHandle m_used;
    MetricHandle m_cache;
    MetricHandle m_usedBytes;
    MetricHandle m_totalBytes;
    MetricHandle m_swapUsed;
    MetricHandle m_swapUsedBytes;
    MetricHandle m_swapTotalBytes;
    bool m_hasSwap = false;
    bool m_valid;
};

} // namespace qk::core
