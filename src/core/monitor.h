// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace qk::core {

/// A data source sampled by the Sampler. Implementations register their metrics in the
/// MetricRegistry at construction and push values in sample(), which must not allocate.
class Monitor
{
public:
    Monitor() = default;
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;
    Monitor(Monitor&&) = delete;
    Monitor& operator=(Monitor&&) = delete;
    virtual ~Monitor() = default;

    /// Reads the source and pushes new values. Hot path: no heap allocation.
    virtual void sample() = 0;

    /// Sample every N base ticks of the Sampler (1 = every tick).
    [[nodiscard]] virtual int periodTicks() const { return 1; }
};

} // namespace qk::core
