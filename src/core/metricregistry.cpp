// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/metricregistry.h"

#include <utility>

namespace qk::core {

MetricRegistry::MetricRegistry(std::size_t historyLength)
    : m_historyLength(historyLength)
{}

MetricHandle MetricRegistry::add(MetricInfo info)
{
    if (const auto existing = find(info.id)) {
        return *existing;
    }
    m_metrics.push_back(
        Entry{.info = std::move(info), .history = RingBuffer<float>(m_historyLength)});
    return static_cast<MetricHandle>(m_metrics.size() - 1);
}

std::optional<MetricHandle> MetricRegistry::find(std::string_view id) const
{
    for (std::size_t i = 0; i < m_metrics.size(); ++i) {
        if (m_metrics[i].info.id == id) {
            return static_cast<MetricHandle>(i);
        }
    }
    return std::nullopt;
}

void MetricRegistry::push(MetricHandle handle, float value) noexcept
{
    m_metrics[handle].history.push(value);
}

const MetricInfo& MetricRegistry::info(MetricHandle handle) const
{
    return m_metrics.at(handle).info;
}

const RingBuffer<float>& MetricRegistry::history(MetricHandle handle) const
{
    return m_metrics.at(handle).history;
}

} // namespace qk::core
