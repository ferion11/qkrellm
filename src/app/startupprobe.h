// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>

namespace qk::app {

/// Milliseconds since this process was started by the kernel (from /proc/self/stat
/// `starttime` vs CLOCK_BOOTTIME). Resolution is one clock tick (usually 10 ms).
/// Used by `--exit-after-first-paint` for the startup budget (docs/resource-budget.md).
[[nodiscard]] std::optional<long long> msSinceProcessStart();

} // namespace qk::app
