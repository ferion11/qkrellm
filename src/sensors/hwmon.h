// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace qk::sensors {

/// One hwmon input file found at startup.
/// Kernel docs: Documentation/hwmon/sysfs-interface.rst. Each /sys/class/hwmon/hwmonN has
/// a "name" (chip driver), optional "device" link, and per-channel files
/// "temp<i>_input" (millidegrees Celsius), "fan<i>_input" (RPM) with optional
/// "<type><i>_label".
struct HwmonSensor
{
    enum class Kind : std::uint8_t { Temperature, Fan };

    Kind kind = Kind::Temperature;
    int hwmon = 0;         ///< N of hwmonN (discovery order)
    int channel = 0;       ///< i of temp<i>_input / fan<i>_input
    std::string chip;      ///< contents of "name", e.g. "coretemp", "nvme"
    std::string device;    ///< basename of the "device" link target, e.g. "nvme1"; may be empty
    std::string label;     ///< "<type><i>_label" contents, or "temp<i>" / "fan<i>"
    std::string inputPath; ///< absolute path of the *_input file
};

/// Lists temperature and fan inputs under `root` (normally /sys/class/hwmon), ordered by
/// hwmon number, then temperatures before fans, then channel. Unreadable or malformed
/// entries are skipped. Does filesystem I/O and allocates: call at startup only.
[[nodiscard]] std::vector<HwmonSensor> discoverHwmon(const std::string& root);

} // namespace qk::sensors
