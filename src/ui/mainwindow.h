// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"

#include <QWidget>

#include <memory>

namespace qk::ui {

class TextRenderer;

/// Top-level krell stack. M1 placeholder: app name + current CPU usage as text
/// (charts arrive with ChartWidget/Panel).
class MainWindow final : public QWidget
{
    Q_OBJECT

public:
    /// `registry` must outlive the window; `cpuBusy` is a Fraction metric.
    MainWindow(const core::MetricRegistry& registry, core::MetricHandle cpuBusy,
               QWidget* parent = nullptr);
    ~MainWindow() override;
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

signals:
    /// Emitted once, after the first paintEvent completes (used by the startup benchmark).
    void firstPaintDone();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const core::MetricRegistry* m_registry;
    core::MetricHandle m_cpuBusy;
    std::unique_ptr<TextRenderer> m_text;
    bool m_painted = false;
};

} // namespace qk::ui
