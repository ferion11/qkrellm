// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/theme.h"

#include <QColor>
#include <QWidget>

#include <climits>
#include <cstdint>
#include <vector>

namespace qk::ui {

class TextRenderer;

struct ChartSeries
{
    core::MetricHandle metric = 0; ///< any value >= 0, drawn relative to the chart's scale
    QColor color;
};

/// What full chart height means (ADR 0012).
enum class ChartScale : std::uint8_t {
    Fixed, ///< full height = a given maximum (1.0 for fractions, 100 for °C)
    Auto,  ///< power-of-two ceiling of the visible stacked maximum (rates, bytes/s)
};

/// ADR 0012: smallest power of two >= `maximum`, at least 1024 (1 KiB). NaN/negative -> 1024.
[[nodiscard]] double chartAutoScale(double maximum);

/// Scrolling history chart: one column per sample, newest at the right edge. Series are
/// stacked bottom-up in the order given (their sum is clipped at the top). Painting reads
/// the registry's ring buffers directly and uses only fillRect: no per-frame allocation.
/// With ChartScale::Auto and a text renderer, the current scale is drawn top-left.
class ChartWidget final : public QWidget
{
    Q_OBJECT

public:
    ChartWidget(const core::MetricRegistry& registry, std::vector<ChartSeries> series,
                const Theme& theme, ChartScale scale = ChartScale::Fixed, double fixedMaximum = 1.0,
                const TextRenderer* text = nullptr, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;
    /// Schedules a repaint after new samples: nothing if the chart would look exactly the
    /// same; when exactly one sample arrived and the scale is unchanged, scrolls the painted
    /// pixels one column left (QWidget::scroll) so only the new column (and the scale label)
    /// is painted; otherwise a full repaint. No allocation.
    void refresh();

    /// Full-height value used by the last paint (the fixed maximum for ChartScale::Fixed).
    [[nodiscard]] double lastScale() const { return m_lastScale; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] float sampleAt(const ChartSeries& series, std::size_t age) const;
    /// Calls bar(x, y, height, color) for every bar in columns [xMin, xMax]; returns the
    /// scale used (computed over all visible columns).
    template<typename Bar>
    double layoutBars(const Bar& bar, int xMin = 0, int xMax = INT_MAX) const;
    [[nodiscard]] QRect scaleLabelRect(double scale) const;

    const core::MetricRegistry* m_registry;
    std::vector<ChartSeries> m_series;
    const Theme* m_theme;
    ChartScale m_scale;
    double m_fixedMaximum;
    const TextRenderer* m_text;
    double m_lastScale = 1.0;
    std::uint64_t m_contentKey = 0; ///< what the last refresh() scheduled for painting
    // State at the last refresh(), to decide between scrolling and a full repaint.
    std::uint64_t m_refreshedPushes = 0;
    double m_refreshedScale = 0.0;
    QSize m_refreshedSize;
};

} // namespace qk::ui
