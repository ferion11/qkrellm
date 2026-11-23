// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/diskmonitor.h"

#include "sensors/diskstats.h"
#include "sensors/rate.h"

#include <algorithm>
#include <memory>
#include <string_view>

#include <dirent.h>
#include <sys/stat.h>

namespace qk::core {

namespace {

constexpr std::size_t kStatCapacity = 512; // one line of ~17 numbers

bool exists(const std::string& path)
{
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

std::string join(std::string_view a, std::string_view b, std::string_view c = {})
{
    std::string path;
    path.reserve(a.size() + b.size() + c.size() + 1);
    path += a;
    path += '/';
    path += b;
    path += c;
    return path;
}

std::vector<std::string> listDir(const std::string& path)
{
    std::vector<std::string> names;
    const std::unique_ptr<DIR, int (*)(DIR*)> dir(::opendir(path.c_str()), ::closedir);
    if (!dir) {
        return names;
    }
    // Startup-only and single-threaded: readdir's static state is not shared.
    while (const dirent* entry = ::readdir(dir.get())) { // NOLINT(concurrency-mt-unsafe)
        const std::string_view name(entry->d_name);
        if (!name.starts_with('.')) {
            names.emplace_back(name);
        }
    }
    std::ranges::sort(names);
    return names;
}

} // namespace

DiskMonitor::DiskMonitor(MetricRegistry& registry, const std::string& sysBlock)
    : m_registry(&registry)
    , m_read(registry.add(
          {.id = "disk.read", .label = "Disk read", .unit = MetricUnit::BytesPerSecond}))
    , m_write(registry.add(
          {.id = "disk.write", .label = "Disk write", .unit = MetricUnit::BytesPerSecond}))
    , m_total(
          registry.add({.id = "disk.total", .label = "Disk", .unit = MetricUnit::BytesPerSecond}))
    , m_previousTime(Clock::now())
{
    const std::vector<std::string> entries = listDir(sysBlock);
    std::vector<std::string> chosen;
    for (const std::string& name : entries) {
        if (exists(join(sysBlock, name, "/device"))) {
            chosen.push_back(name);
        }
    }
    if (chosen.empty()) {
        for (const std::string& name : entries) {
            if (!name.starts_with("loop") && !name.starts_with("ram")) {
                chosen.push_back(name);
            }
        }
    }

    for (const std::string& name : chosen) {
        sensors::ProcFile file(join(sysBlock, name, "/stat").c_str(), kStatCapacity);
        const auto text = file.read();
        const auto stat = text ? sensors::parseBlockStat(*text) : std::nullopt;
        if (!stat) {
            continue; // no readable stat: not a disk we can follow
        }
        m_disks.push_back({
            .name = name,
            .read = registry.add({
                .id = "disk." + name + ".read",
                .label = name + " read",
                .unit = MetricUnit::BytesPerSecond,
            }),
            .write = registry.add({
                .id = "disk." + name + ".write",
                .label = name + " write",
                .unit = MetricUnit::BytesPerSecond,
            }),
        });
        m_files.push_back(std::move(file));
        m_previous.push_back({
            .sectorsRead = stat->sectorsRead,
            .sectorsWritten = stat->sectorsWritten,
            .present = true,
        });
    }
}

DiskMonitor::~DiskMonitor() = default;

void DiskMonitor::sample()
{
    sampleAt(Clock::now());
}

void DiskMonitor::sampleAt(Clock::time_point now)
{
    if (m_disks.empty()) {
        return;
    }
    const double seconds = std::chrono::duration<double>(now - m_previousTime).count();
    m_previousTime = now;
    constexpr auto kSector = static_cast<double>(sensors::DiskStat::kSectorBytes);

    double readSum = 0.0;
    double writeSum = 0.0;
    for (std::size_t k = 0; k < m_disks.size(); ++k) {
        Counters& previous = m_previous[k];
        const auto text = m_files[k].read();
        const auto current = text ? sensors::parseBlockStat(*text) : std::nullopt;
        double read = 0.0;
        double write = 0.0;
        if (current) {
            if (previous.present) {
                read = sensors::counterRate(previous.sectorsRead, current->sectorsRead, seconds) *
                       kSector;
                write = sensors::counterRate(previous.sectorsWritten, current->sectorsWritten,
                                             seconds) *
                        kSector;
            }
            previous = {
                .sectorsRead = current->sectorsRead,
                .sectorsWritten = current->sectorsWritten,
                .present = true,
            };
        } else {
            previous.present = false; // gone: next appearance starts a new baseline
        }
        m_registry->push(m_disks[k].read, static_cast<float>(read));
        m_registry->push(m_disks[k].write, static_cast<float>(write));
        readSum += read;
        writeSum += write;
    }
    m_registry->push(m_read, static_cast<float>(readSum));
    m_registry->push(m_write, static_cast<float>(writeSum));
    m_registry->push(m_total, static_cast<float>(readSum + writeSum));
}

} // namespace qk::core
