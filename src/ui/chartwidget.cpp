// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/chartwidget.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <utility>

namespace qk::ui {

namespace {
constexpr int kMinimumWidth = 32;
} // namespace

ChartWidget::ChartWidget(const core::MetricRegistry& registry, std::vector<ChartSeries> series,
                         const Theme& theme, QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_series(std::move(series))
    , m_theme(&theme)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QSize ChartWidget::sizeHint() const
{
    return {m_theme->panelWidth, m_theme->chartHeight};
}

QSize ChartWidget::minimumSizeHint() const
{
    return {kMinimumWidth, m_theme->chartHeight};
}

void ChartWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const int w = width();
    const int h = height();
    painter.fillRect(rect(), m_theme->chartBackground);
    painter.fillRect(0, h / 2, w, 1, m_theme->chartGrid); // 50 % guide

    std::size_t samples = m_registry->historyLength();
    for (const ChartSeries& s : m_series) {
        samples = std::min(samples, m_registry->history(s.metric).size());
    }
    const auto columns = std::min(samples, static_cast<std::size_t>(std::max(w, 0)));

    for (std::size_t k = 0; k < columns; ++k) {
        const int x = w - 1 - static_cast<int>(k);
        int stacked = 0;
        for (const ChartSeries& s : m_series) {
            const auto& history = m_registry->history(s.metric);
            const float value = std::clamp(history[history.size() - 1 - k], 0.0F, 1.0F);
            const int bar =
                std::min(static_cast<int>(std::lround(value * static_cast<float>(h))), h - stacked);
            if (bar > 0) {
                painter.fillRect(x, h - stacked - bar, 1, bar, s.color);
                stacked += bar;
            }
        }
    }
}

} // namespace qk::ui
