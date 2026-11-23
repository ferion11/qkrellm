// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/sysfsvalue.h"

#include "sensors/textscan.h"

#include <limits>

namespace qk::sensors {

std::optional<std::int64_t> parseSysfsInteger(std::string_view text)
{
    const auto end = text.find_last_not_of(" \t\n\r");
    if (end == std::string_view::npos) {
        return std::nullopt;
    }
    text = text.substr(0, end + 1);
    const bool negative = text.starts_with('-');
    if (negative) {
        text.remove_prefix(1);
    }
    const auto magnitude = parseUnsigned(text);
    if (!magnitude) {
        return std::nullopt;
    }
    constexpr auto kMax = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (negative) {
        if (*magnitude > kMax + 1) {
            return std::nullopt;
        }
        // -(2^63) is representable even though +2^63 is not.
        return *magnitude == kMax + 1 ? std::numeric_limits<std::int64_t>::min()
                                      : -static_cast<std::int64_t>(*magnitude);
    }
    if (*magnitude > kMax) {
        return std::nullopt;
    }
    return static_cast<std::int64_t>(*magnitude);
}

} // namespace qk::sensors
