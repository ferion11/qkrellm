// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/filesystemmonitor.h"

#include "sensors/procfile.h"

#include <utility>

#include <sys/statvfs.h>

namespace qk::core {

namespace {
constexpr std::size_t kMountsCapacity = std::size_t{16} * 1024;
} // namespace

std::optional<sensors::FsUsage> FilesystemMonitor::statvfs(const std::string& path)
{
    struct statvfs st{};
    if (::statvfs(path.c_str(), &st) != 0) {
        return std::nullopt;
    }
    return sensors::fsUsage(st.f_blocks, st.f_bfree, st.f_bavail, st.f_frsize);
}

FilesystemMonitor::FilesystemMonitor(MetricRegistry& registry, const std::string& mountsPath,
                                     StatFn stat)
    : m_registry(&registry)
    , m_stat(stat ? std::move(stat) : StatFn(&FilesystemMonitor::statvfs))
{
    sensors::ProcFile file(mountsPath.c_str(), kMountsCapacity);
    const auto text = file.read();
    if (!text) {
        return;
    }
    for (const sensors::MountEntry& m : sensors::selectDiskMounts(sensors::parseMounts(*text))) {
        const std::string id = "fs:" + m.mountPoint;
        m_mounts.push_back({
            .mountPoint = m.mountPoint,
            .used = registry.add(
                {.id = id + ".used", .label = m.mountPoint, .unit = MetricUnit::Fraction}),
            .usedBytes = registry.add(
                {.id = id + ".used.bytes", .label = m.mountPoint, .unit = MetricUnit::Bytes}),
            .totalBytes = registry.add(
                {.id = id + ".total.bytes", .label = m.mountPoint, .unit = MetricUnit::Bytes}),
        });
    }
    sample();
}

void FilesystemMonitor::sample()
{
    for (const Mount& m : m_mounts) {
        const auto usage = m_stat(m.mountPoint);
        if (!usage) {
            continue; // unmounted or unreachable: no new point
        }
        m_registry->push(m.used, static_cast<float>(usage->fraction));
        m_registry->push(m.usedBytes, static_cast<float>(usage->used));
        m_registry->push(m.totalBytes, static_cast<float>(usage->total));
    }
}

} // namespace qk::core
