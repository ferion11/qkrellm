// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/theme.h"

#include <QColor>
#include <QWidget>

#include <cstdint>
#include <vector>

namespace qk::ui {

class TextRenderer;

struct ChartSeries
{
    core::MetricHandle metric = 0; ///< Fraction (0..1) or, with ChartScale::Auto, any >= 0
    QColor color;
};

/// What full chart height means (ADR 0012).
enum class ChartScale : std::uint8_t {
    Fraction, ///< fixed: 1.0
    Auto,     ///< power-of-two ceiling of the visible stacked maximum (rates, bytes/s)
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
                const Theme& theme, ChartScale scale = ChartScale::Fraction,
                const TextRenderer* text = nullptr, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;
    /// Full-height value used by the last paint (1.0 for ChartScale::Fraction).
    [[nodiscard]] double lastScale() const { return m_lastScale; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] float sampleAt(const ChartSeries& series, std::size_t age) const;

    const core::MetricRegistry* m_registry;
    std::vector<ChartSeries> m_series;
    const Theme* m_theme;
    ChartScale m_scale;
    const TextRenderer* m_text;
    double m_lastScale = 1.0;
};

} // namespace qk::ui
