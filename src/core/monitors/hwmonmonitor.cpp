// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/hwmonmonitor.h"

#include "sensors/sysfsvalue.h"

#include <algorithm>
#include <utility>

namespace qk::core {

namespace {

using sensors::HwmonSensor;

constexpr std::size_t kValueCapacity = 64; // "-123456\n"
constexpr double kMilli = 1000.0;

bool isCpuTemperature(const HwmonSensor& s)
{
    if (s.kind != HwmonSensor::Kind::Temperature) {
        return false;
    }
    if (s.chip == "coretemp") {
        return s.label.starts_with("Package id");
    }
    if (s.chip == "k10temp" || s.chip == "zenpower") {
        return s.label == "Tctl" || s.label == "Tdie";
    }
    return s.chip == "cpu_thermal"; // Raspberry Pi and other SoCs
}

} // namespace

HwmonMonitor::HwmonMonitor(MetricRegistry& registry, const std::string& root,
                           std::chrono::microseconds slowThreshold)
    : m_registry(&registry)
    , m_sensors(sensors::discoverHwmon(root))
    , m_slowThreshold(slowThreshold)
{
    m_active.reserve(m_sensors.size()); // enable() never reallocates under the sampler
}

std::optional<MetricHandle> HwmonMonitor::enable(std::size_t index)
{
    if (index >= m_sensors.size()) {
        return std::nullopt;
    }
    for (const Active& a : m_active) {
        if (a.index == index) {
            return a.handle;
        }
    }
    const HwmonSensor& s = m_sensors[index];
    const bool temp = s.kind == HwmonSensor::Kind::Temperature;
    const std::string where = s.device.empty() ? "hwmon" + std::to_string(s.hwmon) : s.device;
    const MetricHandle handle = m_registry->add({
        .id = "hwmon." + s.chip + "." + where + "." + (temp ? "temp" : "fan") +
              std::to_string(s.channel),
        .label = displayName(index) + " " + s.label,
        .unit = temp ? MetricUnit::Celsius : MetricUnit::Rpm,
    });
    m_active.push_back({
        .index = index,
        .handle = handle,
        .file = sensors::ProcFile(s.inputPath.c_str(), kValueCapacity),
    });
    return handle;
}

void HwmonMonitor::sample()
{
    for (Active& a : m_active) {
        if (a.period == 0) {
            const auto start = std::chrono::steady_clock::now();
            read(a);
            const auto spent = std::chrono::steady_clock::now() - start;
            a.period = spent > m_slowThreshold ? kSlowPeriodTicks : 1;
        } else if (m_tick % static_cast<std::uint64_t>(a.period) == 0) {
            read(a);
        }
    }
    ++m_tick;
}

int HwmonMonitor::periodTicksOf(std::size_t index) const
{
    for (const Active& a : m_active) {
        if (a.index == index) {
            return a.period;
        }
    }
    return 0;
}

void HwmonMonitor::read(Active& active)
{
    const auto text = active.file.read();
    if (!text) {
        return; // sensor gone or busy: no new point
    }
    const auto raw = sensors::parseSysfsInteger(*text);
    if (!raw) {
        return;
    }
    const bool temp = m_sensors[active.index].kind == HwmonSensor::Kind::Temperature;
    const double value = temp ? static_cast<double>(*raw) / kMilli : static_cast<double>(*raw);
    m_registry->push(active.handle, static_cast<float>(value));
}

std::string HwmonMonitor::displayName(std::size_t index) const
{
    const HwmonSensor& s = m_sensors.at(index);
    const bool shared = std::ranges::any_of(
        m_sensors, [&](const HwmonSensor& o) { return o.chip == s.chip && o.hwmon != s.hwmon; });
    return shared && !s.device.empty() ? s.device : s.chip;
}

std::optional<std::size_t> HwmonMonitor::cpuTemperature() const
{
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        if (isCpuTemperature(m_sensors[i])) {
            return i;
        }
    }
    return std::nullopt;
}

std::vector<std::size_t> HwmonMonitor::defaultSelection() const
{
    std::vector<std::size_t> selection;
    const auto cpu = cpuTemperature();
    if (cpu) {
        selection.push_back(*cpu);
    }
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        const HwmonSensor& s = m_sensors[i];
        if (s.kind != HwmonSensor::Kind::Temperature || (cpu && s.hwmon == m_sensors[*cpu].hwmon)) {
            continue;
        }
        const bool firstOfChip = std::ranges::none_of(
            selection, [&](std::size_t j) { return m_sensors[j].hwmon == s.hwmon; });
        if (firstOfChip) {
            selection.push_back(i);
        }
    }
    for (std::size_t i = 0; i < m_sensors.size(); ++i) {
        if (m_sensors[i].kind == HwmonSensor::Kind::Fan) {
            selection.push_back(i);
        }
    }
    return selection;
}

} // namespace qk::core
