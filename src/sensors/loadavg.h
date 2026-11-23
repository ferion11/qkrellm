// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string_view>

namespace qk::sensors {

/// The three load averages of /proc/loadavg ("0.63 0.67 2.27 2/1177 174251").
/// Kernel: fs/proc/loadavg.c; always two decimals. Later fields are ignored.
/// Fixtures: tests/fixtures/proc/<tag>/proc/loadavg (real captures).
struct LoadAvg
{
    double one = 0.0;
    double five = 0.0;
    double fifteen = 0.0;
};

/// Parses /proc/loadavg; std::nullopt unless the first three fields are plain non-negative
/// decimals ("12", "0.63"). Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<LoadAvg> parseLoadAvg(std::string_view text);

} // namespace qk::sensors
