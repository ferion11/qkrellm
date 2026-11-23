// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace qk::sensors {

/// Units per second between two readings of a monotonically increasing counter.
/// A counter that went backwards (interface re-created, driver reset, 32-bit wrap) yields 0
/// for that interval rather than a huge bogus rate. Non-positive `seconds` yields 0.
[[nodiscard]] double counterRate(std::uint64_t previous, std::uint64_t current, double seconds);

} // namespace qk::sensors
