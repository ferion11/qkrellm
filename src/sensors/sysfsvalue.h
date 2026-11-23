// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// Parses a sysfs attribute holding one decimal integer, e.g. hwmon "temp1_input" =
/// "42850\n" (millidegrees Celsius) or "-5000\n". Accepts an optional leading '-' and
/// trailing whitespace (the kernel appends '\n'); anything else -> std::nullopt
/// (empty, garbage, overflow of int64).
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<std::int64_t> parseSysfsInteger(std::string_view text);

} // namespace qk::sensors
