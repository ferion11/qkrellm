// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/chartwidget.h"

#include "ui/textformat.h"
#include "ui/textrenderer.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <cmath>
#include <utility>

namespace qk::ui {

namespace {
constexpr int kMinimumWidth = 32;
constexpr double kMinAutoScale = 1024.0;
} // namespace

double chartAutoScale(double maximum)
{
    double scale = kMinAutoScale;
    while (scale < maximum && scale < 1e300) { // NaN compares false: stays at the floor
        scale *= 2.0;
    }
    return scale;
}

ChartWidget::ChartWidget(const core::MetricRegistry& registry, std::vector<ChartSeries> series,
                         const Theme& theme, ChartScale scale, double fixedMaximum,
                         const TextRenderer* text, QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_series(std::move(series))
    , m_theme(&theme)
    , m_scale(scale)
    , m_fixedMaximum(fixedMaximum > 0.0 ? fixedMaximum : 1.0)
    , m_text(text)
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

float ChartWidget::sampleAt(const ChartSeries& series, std::size_t age) const
{
    const auto& history = m_registry->history(series.metric);
    const float value = history[history.size() - 1 - age];
    return std::isnan(value) ? 0.0F : std::max(value, 0.0F);
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

    double scale = m_fixedMaximum;
    if (m_scale == ChartScale::Auto) {
        double maximum = 0.0;
        for (std::size_t k = 0; k < columns; ++k) {
            double stacked = 0.0;
            for (const ChartSeries& s : m_series) {
                stacked += static_cast<double>(sampleAt(s, k));
            }
            maximum = std::max(maximum, stacked);
        }
        scale = chartAutoScale(maximum);
    }
    m_lastScale = scale;

    for (std::size_t k = 0; k < columns; ++k) {
        const int x = w - 1 - static_cast<int>(k);
        int stacked = 0;
        for (const ChartSeries& s : m_series) {
            const double value = std::min(static_cast<double>(sampleAt(s, k)) / scale, 1.0);
            const int bar = std::min(static_cast<int>(std::lround(value * h)), h - stacked);
            if (bar > 0) {
                painter.fillRect(x, h - stacked - bar, 1, bar, s.color);
                stacked += bar;
            }
        }
    }

    if (m_scale == ChartScale::Auto && m_text != nullptr) {
        TextBuffer buffer{};
        m_text->draw(painter, QPoint(1, 0), formatBytes(buffer, scale), m_theme->dimText);
    }
}

} // namespace qk::ui
