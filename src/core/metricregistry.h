// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/ringbuffer.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace qk::core {

enum class MetricUnit : std::uint8_t {
    Fraction, ///< 0..1 (CPU busy, memory used)
    Count,
    Bytes,
    BytesPerSecond,
    Celsius,
};

struct MetricInfo
{
    std::string id;    ///< stable dotted id, e.g. "cpu.busy", "cpu3.busy"
    std::string label; ///< short display label, e.g. "CPU", "CPU3"
    MetricUnit unit = MetricUnit::Fraction;
};

/// Index of a metric in the registry; cheap to store and use in the hot path.
using MetricHandle = std::uint32_t;

/// Owns every metric's description and history. Registration (setup time) allocates;
/// push() and the read accessors never do.
class MetricRegistry
{
public:
    explicit MetricRegistry(std::size_t historyLength);

    /// Registers a metric; ids must be unique (a duplicate returns the existing handle).
    MetricHandle add(MetricInfo info);
    [[nodiscard]] std::optional<MetricHandle> find(std::string_view id) const;

    /// Hot path: appends a sample. Precondition: valid handle.
    void push(MetricHandle handle, float value) noexcept;

    [[nodiscard]] const MetricInfo& info(MetricHandle handle) const;
    [[nodiscard]] const RingBuffer<float>& history(MetricHandle handle) const;
    [[nodiscard]] std::size_t size() const { return m_metrics.size(); }
    [[nodiscard]] std::size_t historyLength() const { return m_historyLength; }

private:
    struct Entry
    {
        MetricInfo info;
        RingBuffer<float> history;
    };
    std::size_t m_historyLength;
    std::vector<Entry> m_metrics;
};

} // namespace qk::core
