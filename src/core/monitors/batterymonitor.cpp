// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/batterymonitor.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <span>

#include <dirent.h>

namespace qk::core {

namespace {

constexpr std::size_t kUeventCapacity = 4096;

std::vector<std::string> supplyDirs(const std::string& root)
{
    std::vector<std::string> names;
    const std::unique_ptr<DIR, int (*)(DIR*)> dir(::opendir(root.c_str()), ::closedir);
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
    std::ranges::sort(names); // stable order (BAT0 before BAT1)
    return names;
}

} // namespace

BatteryMonitor::BatteryMonitor(MetricRegistry& registry, const std::string& root)
    : m_registry(&registry)
    , m_charge(registry.add({.id = "battery.charge", .label = "Bat", .unit = MetricUnit::Fraction}))
    , m_seconds(
          registry.add({.id = "battery.seconds", .label = "Bat time", .unit = MetricUnit::Count}))
    , m_state(
          registry.add({.id = "battery.state", .label = "Bat state", .unit = MetricUnit::Count}))
    , m_ac(registry.add({.id = "power.ac", .label = "AC", .unit = MetricUnit::Count}))
{
    for (const std::string& name : supplyDirs(root)) {
        if (m_files.size() == kMaxSupplies) {
            break;
        }
        std::string path = root;
        path += '/';
        path += name;
        path += "/uevent";
        sensors::ProcFile file(path.c_str(), kUeventCapacity);
        const auto text = file.read();
        if (!text) {
            continue;
        }
        const auto supply = sensors::parsePowerSupplyUevent(*text);
        using Type = sensors::PowerSupply::Type;
        const bool battery = supply.type == Type::Battery && !supply.deviceScope;
        if (battery || supply.type == Type::Mains || supply.type == Type::Usb) {
            m_hasBattery = m_hasBattery || battery;
            m_files.push_back(std::move(file));
        }
    }
    read();
}

void BatteryMonitor::sample()
{
    read();
}

void BatteryMonitor::read()
{
    std::size_t count = 0;
    for (sensors::ProcFile& file : m_files) {
        if (const auto text = file.read()) {
            m_parsed.at(count++) = sensors::parsePowerSupplyUevent(*text);
        }
    }
    const auto e = sensors::estimateBattery(std::span(m_parsed.data(), count));
    m_registry->push(m_ac, e.acOnline ? 1.0F : 0.0F);
    if (!e.hasBattery) {
        return;
    }
    m_registry->push(m_charge, e.fraction ? static_cast<float>(*e.fraction)
                                          : std::numeric_limits<float>::quiet_NaN());
    m_registry->push(m_seconds, e.secondsLeft ? static_cast<float>(*e.secondsLeft)
                                              : std::numeric_limits<float>::quiet_NaN());
    m_registry->push(m_state, static_cast<float>(static_cast<int>(e.state)));
}

} // namespace qk::core
