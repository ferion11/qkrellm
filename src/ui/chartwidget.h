// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/theme.h"

#include <QColor>
#include <QWidget>

#include <vector>

namespace qk::ui {

struct ChartSeries
{
    core::MetricHandle metric = 0; ///< a MetricUnit::Fraction metric (0..1)
    QColor color;
};

/// Scrolling history chart: one column per sample, newest at the right edge. Series are
/// stacked bottom-up in the order given (their sum is clipped at the top). Painting reads
/// the registry's ring buffers directly and uses only fillRect: no per-frame allocation.
class ChartWidget final : public QWidget
{
    Q_OBJECT

public:
    ChartWidget(const core::MetricRegistry& registry, std::vector<ChartSeries> series,
                const Theme& theme, QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    const core::MetricRegistry* m_registry;
    std::vector<ChartSeries> m_series;
    const Theme* m_theme;
};

} // namespace qk::ui
