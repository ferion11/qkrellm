// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/diskmonitor.h"

#include "sensors/rate.h"

#include <sys/stat.h>

namespace qk::core {

namespace {

bool exists(const std::string& path)
{
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

} // namespace

DiskMonitor::DiskMonitor(MetricRegistry& registry, const char* path, const char* sysBlock)
    : m_registry(&registry)
    , m_file(path, std::size_t{8} * 1024)
    , m_current(std::make_unique<sensors::DiskStats>())
    , m_read(registry.add(
          {.id = "disk.read", .label = "Disk read", .unit = MetricUnit::BytesPerSecond}))
    , m_write(registry.add(
          {.id = "disk.write", .label = "Disk write", .unit = MetricUnit::BytesPerSecond}))
    , m_total(
          registry.add({.id = "disk.total", .label = "Disk", .unit = MetricUnit::BytesPerSecond}))
    , m_previousTime(Clock::now())
{
    if (!readInto(*m_current)) {
        return;
    }
    m_valid = true;
    const sensors::DiskStats& stats = *m_current;
    const std::string block(sysBlock);

    std::vector<std::size_t> chosen;
    for (std::size_t i = 0; i < stats.count; ++i) {
        if (exists(block + "/" + std::string(stats.devices.at(i).name()) + "/device")) {
            chosen.push_back(i);
        }
    }
    if (chosen.empty()) {
        for (std::size_t i = 0; i < stats.count; ++i) {
            const auto name = stats.devices.at(i).name();
            if (!name.starts_with("loop") && !name.starts_with("ram") &&
                exists(block + "/" + std::string(name))) {
                chosen.push_back(i);
            }
        }
    }

    for (const std::size_t i : chosen) {
        const sensors::DiskStat& d = stats.devices.at(i);
        const std::string name(d.name());
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
        m_previous.push_back({
            .sectorsRead = d.sectorsRead,
            .sectorsWritten = d.sectorsWritten,
            .present = true,
        });
    }
}

DiskMonitor::~DiskMonitor() = default;

bool DiskMonitor::readInto(sensors::DiskStats& out)
{
    const auto text = m_file.read();
    if (!text) {
        return false;
    }
    out = sensors::parseDiskStats(*text);
    return true;
}

void DiskMonitor::sample()
{
    sampleAt(Clock::now());
}

void DiskMonitor::sampleAt(Clock::time_point now)
{
    if (!m_valid || !readInto(*m_current)) {
        return; // charts simply get no new point
    }
    const double seconds = std::chrono::duration<double>(now - m_previousTime).count();
    m_previousTime = now;
    constexpr auto kSector = static_cast<double>(sensors::DiskStat::kSectorBytes);

    double readSum = 0.0;
    double writeSum = 0.0;
    for (std::size_t k = 0; k < m_disks.size(); ++k) {
        Counters& previous = m_previous[k];
        const auto index = m_current->find(m_disks[k].name);
        double read = 0.0;
        double write = 0.0;
        if (index) {
            const sensors::DiskStat& current = m_current->devices.at(*index);
            if (previous.present) {
                read = sensors::counterRate(previous.sectorsRead, current.sectorsRead, seconds) *
                       kSector;
                write =
                    sensors::counterRate(previous.sectorsWritten, current.sectorsWritten, seconds) *
                    kSector;
            }
            previous = {
                .sectorsRead = current.sectorsRead,
                .sectorsWritten = current.sectorsWritten,
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
