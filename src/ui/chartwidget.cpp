// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/chartwidget.h"

#include "ui/textformat.h"
#include "ui/textrenderer.h"

#include <QPaintEvent>
#include <QPainter>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
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

template<typename Bar>
double ChartWidget::layoutBars(const Bar& bar, int xMin, int xMax) const
{
    const int w = width();
    const int h = height();
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

    for (std::size_t k = 0; k < columns; ++k) {
        const int x = w - 1 - static_cast<int>(k);
        if (x < xMin || x > xMax) {
            continue;
        }
        int stacked = 0;
        for (const ChartSeries& s : m_series) {
            const double value = std::min(static_cast<double>(sampleAt(s, k)) / scale, 1.0);
            const int height = std::min(static_cast<int>(std::lround(value * h)), h - stacked);
            if (height > 0) {
                bar(x, h - stacked - height, height, s.color);
                stacked += height;
            }
        }
    }
    return scale;
}

void ChartWidget::refresh()
{
    // FNV-1a over everything the paint depends on. Scrolling a chart whose visible bars are
    // all alike (idle link, steady memory) yields the same pixels: skip that repaint.
    constexpr std::uint64_t kOffset = 14695981039346656037ULL;
    constexpr std::uint64_t kPrime = 1099511628211ULL;
    std::uint64_t hash = kOffset;
    const auto mix = [&hash](std::uint64_t v) {
        hash ^= v;
        hash *= kPrime;
    };
    const double scale = layoutBars([&](int x, int y, int height, const QColor& color) {
        mix(static_cast<std::uint64_t>(x));
        mix(static_cast<std::uint64_t>(y));
        mix(static_cast<std::uint64_t>(height));
        mix(static_cast<std::uint64_t>(color.rgba()));
    });
    mix(std::bit_cast<std::uint64_t>(scale));
    mix((static_cast<std::uint64_t>(width()) << 32U) | static_cast<std::uint32_t>(height()));

    const std::uint64_t pushes =
        m_series.empty() ? 0 : m_registry->history(m_series.front().metric).pushes();
    const bool oneNewSample = pushes == m_refreshedPushes + 1;
    m_refreshedPushes = pushes;
    if (hash == m_contentKey) {
        return; // identical frame: what is on screen is already right
    }
    const bool canScroll =
        oneNewSample && m_contentKey != 0 && scale == m_refreshedScale && size() == m_refreshedSize;
    m_contentKey = hash;
    m_refreshedScale = scale;
    m_refreshedSize = size();
    if (!canScroll) {
        update();
        return;
    }
    scroll(-1, 0); // Qt moves the pixels and schedules a paint of the new right column
    if (m_scale == ChartScale::Auto && m_text != nullptr) {
        update(scaleLabelRect(scale)); // the label must stay put, not scroll
    }
}

QRect ChartWidget::scaleLabelRect(double scale) const
{
    TextBuffer buffer{};
    const int labelWidth = m_text->horizontalAdvance(formatBytes(buffer, scale));
    return {0, 0, labelWidth + 2, m_text->lineHeight()};
}

void ChartWidget::paintEvent(QPaintEvent* event)
{
    const QRect dirty = event->rect();
    QPainter painter(this);
    painter.fillRect(dirty, m_theme->chartBackground);
    painter.fillRect(dirty.left(), height() / 2, dirty.width(), 1, m_theme->chartGrid); // 50 %
    const double scale =
        layoutBars([&painter](int x, int y, int height,
                              const QColor& color) { painter.fillRect(x, y, 1, height, color); },
                   dirty.left(), dirty.right());
    m_lastScale = scale;

    if (m_scale == ChartScale::Auto && m_text != nullptr &&
        dirty.intersects(scaleLabelRect(scale))) {
        TextBuffer buffer{};
        m_text->draw(painter, QPoint(1, 0), formatBytes(buffer, scale), m_theme->dimText);
    }
}

} // namespace qk::ui
