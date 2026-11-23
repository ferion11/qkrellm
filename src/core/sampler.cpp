// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/sampler.h"

namespace qk::core {

Sampler::Sampler(std::chrono::milliseconds interval, QObject* parent)
    : QObject(parent)
{
    // Coarse timers let the kernel batch wake-ups (±5 %): cheaper than precise timers.
    m_timer.setTimerType(Qt::CoarseTimer);
    m_timer.setInterval(interval);
    connect(&m_timer, &QTimer::timeout, this, &Sampler::tick);
}

Sampler::~Sampler() = default;

void Sampler::addMonitor(std::unique_ptr<Monitor> monitor)
{
    m_monitors.push_back(std::move(monitor));
}

void Sampler::start()
{
    m_timer.start();
}

void Sampler::stop()
{
    m_timer.stop();
}

void Sampler::tick()
{
    ++m_ticks;
    for (const auto& monitor : m_monitors) {
        const auto period = static_cast<std::uint64_t>(monitor->periodTicks());
        if (period <= 1 || m_ticks % period == 0) {
            monitor->sample();
        }
    }
    emit sampled();
}

} // namespace qk::core
