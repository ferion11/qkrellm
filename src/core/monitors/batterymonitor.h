// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/powersupply.h"
#include "sensors/procfile.h"

#include <array>
#include <string>
#include <vector>

namespace qk::core {

/// Battery and AC adapter state from /sys/class/power_supply/*/uevent (see
/// sensors::estimateBattery). Supplies are discovered at construction; every system battery
/// and mains/USB adapter is re-read each period. Registers:
///   "battery.charge"   Fraction of full charge
///   "battery.seconds"  seconds to empty (discharging) or to full (charging); NaN if unknown
///   "battery.state"    sensors::BatteryEstimate::State as a number
///   "power.ac"         1 if an adapter is online, else 0
/// Battery state changes slowly, and the first read of a battery each second triggers an ACPI
/// refresh (~0.4 ms of CPU on the reference laptop): sampled every kPeriodTicks ticks, and
/// once at construction.
class BatteryMonitor final : public Monitor
{
public:
    static constexpr int kPeriodTicks = 10;
    static constexpr std::size_t kMaxSupplies = 16;

    explicit BatteryMonitor(MetricRegistry& registry,
                            const std::string& root = "/sys/class/power_supply");

    void sample() override;
    [[nodiscard]] int periodTicks() const override { return kPeriodTicks; }

    /// A system battery was found at construction.
    [[nodiscard]] bool hasBattery() const { return m_hasBattery; }
    [[nodiscard]] MetricHandle charge() const { return m_charge; }
    [[nodiscard]] MetricHandle seconds() const { return m_seconds; }
    [[nodiscard]] MetricHandle state() const { return m_state; }
    [[nodiscard]] MetricHandle ac() const { return m_ac; }

private:
    void read();

    MetricRegistry* m_registry;
    std::vector<sensors::ProcFile> m_files; // at most kMaxSupplies
    std::array<sensors::PowerSupply, kMaxSupplies> m_parsed{};
    MetricHandle m_charge;
    MetricHandle m_seconds;
    MetricHandle m_state;
    MetricHandle m_ac;
    bool m_hasBattery = false;
};

} // namespace qk::core
