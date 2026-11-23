// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "core/monitor.h"
#include "sensors/hwmon.h"
#include "sensors/procfile.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace qk::core {

/// Temperatures (°C, MetricUnit::Celsius) and fan speeds (MetricUnit::Rpm) from hwmon.
/// Every input is discovered at construction, but only the ones passed to enable() are
/// registered, opened and read: reading some sensors is not free (coretemp wakes the core
/// it measures), so unseen ones are left alone.
/// Some inputs are expensive to read (measured on one laptop: Asus WMI fans ~0.6 ms of CPU
/// each, NVMe temperatures 3–15 ms of wall time as they query the drive). The first read of
/// every enabled sensor is timed; one slower than `slowThreshold` is then read only every
/// `kSlowPeriodTicks` ticks, one slower than 5 × `slowThreshold` every `kVerySlowPeriodTicks`
/// ticks. Cheap ones (coretemp ~5 µs) are read every tick.
/// Metric ids: "hwmon.<chip>.<device or hwmonN>.<temp|fan><channel>", stable across boots
/// unlike hwmonN numbering.
class HwmonMonitor final : public Monitor
{
public:
    static constexpr int kSlowPeriodTicks = 10;
    static constexpr int kVerySlowPeriodTicks = 30;
    static constexpr int kVerySlowFactor = 5;
    static constexpr std::chrono::microseconds kDefaultSlowThreshold{100};

    explicit HwmonMonitor(MetricRegistry& registry, const std::string& root = "/sys/class/hwmon",
                          std::chrono::microseconds slowThreshold = kDefaultSlowThreshold);

    void sample() override;

    [[nodiscard]] const std::vector<sensors::HwmonSensor>& sensors() const { return m_sensors; }

    /// Starts reading sensor `index` (of sensors()) at the next sample() and returns its
    /// metric (not read here: a slow sensor must not delay startup). Enabling twice returns
    /// the same handle. std::nullopt for a bad index. Setup-time only (allocates).
    std::optional<MetricHandle> enable(std::size_t index);

    /// Short display name of sensor `index`'s chip: the chip name, or the device name when
    /// several chips share a name (two "nvme" chips -> "nvme0", "nvme1").
    [[nodiscard]] std::string displayName(std::size_t index) const;

    /// Default selection (indexes into sensors()): the CPU temperature first if one is
    /// recognised (coretemp "Package id N", k10temp/zenpower "Tctl"/"Tdie", cpu_thermal),
    /// then the first temperature of every other chip, then every fan.
    [[nodiscard]] std::vector<std::size_t> defaultSelection() const;
    /// Index of the recognised CPU temperature, if any.
    [[nodiscard]] std::optional<std::size_t> cpuTemperature() const;
    /// Read period, in ticks, of an enabled sensor (0 if not enabled or not read yet).
    [[nodiscard]] int periodTicksOf(std::size_t index) const;

private:
    struct Active
    {
        std::size_t index = 0;
        MetricHandle handle = 0;
        sensors::ProcFile file;
        int period = 0; ///< 0 = not read yet (the first read decides)
    };

    void read(Active& active);

    MetricRegistry* m_registry;
    std::vector<sensors::HwmonSensor> m_sensors;
    std::vector<Active> m_active;
    std::chrono::microseconds m_slowThreshold;
    std::uint64_t m_tick = 0;
};

} // namespace qk::core
