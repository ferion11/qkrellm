// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/monitor.h"

#include <QObject>
#include <QTimer>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace qk::core {

/// Drives every monitor from ONE coarse timer (architecture.md). Each tick samples the
/// monitors whose period divides the tick count, then emits sampled().
class Sampler final : public QObject
{
    Q_OBJECT

public:
    explicit Sampler(std::chrono::milliseconds interval, QObject* parent = nullptr);
    ~Sampler() override;
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;
    Sampler(Sampler&&) = delete;
    Sampler& operator=(Sampler&&) = delete;

    /// Takes ownership. Call during setup, before start().
    void addMonitor(std::unique_ptr<Monitor> monitor);

    /// Starts the timer. Does not sample immediately: monitors take their baseline in their
    /// constructors, so the first tick already yields values.
    void start();
    void stop();

    /// One sampling round (called by the timer; public for tests and benchmarks).
    void tick();

    [[nodiscard]] std::uint64_t ticks() const { return m_ticks; }
    [[nodiscard]] std::chrono::milliseconds interval() const
    {
        return m_timer.intervalAsDuration();
    }

signals:
    void sampled();

private:
    QTimer m_timer;
    std::vector<std::unique_ptr<Monitor>> m_monitors;
    std::uint64_t m_ticks = 0;
};

} // namespace qk::core
