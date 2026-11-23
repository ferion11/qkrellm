// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/mounts.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qk::core {

/// Space used on local disk filesystems (sensors::selectDiskMounts of /proc/self/mounts,
/// read once at construction). Per mount point it registers "fs:<mountpoint>.used"
/// (Fraction, like df's Use%), ".used.bytes" and ".total.bytes" (Bytes).
/// statvfs is cheap but usage changes slowly: sampled every kPeriodTicks ticks, and once
/// at construction. Mounts added later are not followed.
class FilesystemMonitor final : public Monitor
{
public:
    static constexpr int kPeriodTicks = 10;

    /// statvfs(3) wrapper, replaceable in tests; std::nullopt on failure.
    using StatFn = std::function<std::optional<sensors::FsUsage>(const std::string& path)>;

    explicit FilesystemMonitor(MetricRegistry& registry,
                               const std::string& mountsPath = "/proc/self/mounts",
                               StatFn stat = {});

    void sample() override;
    [[nodiscard]] int periodTicks() const override { return kPeriodTicks; }

    struct Mount
    {
        std::string mountPoint;
        MetricHandle used = 0;
        MetricHandle usedBytes = 0;
        MetricHandle totalBytes = 0;
    };
    [[nodiscard]] const std::vector<Mount>& mounts() const { return m_mounts; }

    /// The real statvfs(3).
    [[nodiscard]] static std::optional<sensors::FsUsage> statvfs(const std::string& path);

private:
    MetricRegistry* m_registry;
    StatFn m_stat;
    std::vector<Mount> m_mounts;
};

} // namespace qk::core
