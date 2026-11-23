// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/chartwidget.h"
#include "ui/theme.h"

#include <QString>
#include <QWidget>

#include <optional>
#include <vector>

namespace qk::ui {

class TextRenderer;

struct PanelSpec
{
    QString label;                                 ///< left side of the header, e.g. "CPU"
    std::optional<core::MetricHandle> valueMetric; ///< Fraction shown as "nn%" on the right
    std::vector<ChartSeries> series;               ///< empty = header only
};

/// One monitor in the stack: a text header (label + current value) over an optional chart.
class Panel final : public QWidget
{
    Q_OBJECT

public:
    Panel(const core::MetricRegistry& registry, PanelSpec spec, const TextRenderer& text,
          const Theme& theme, QWidget* parent = nullptr);

    /// Repaints after new samples (header value and chart).
    void refresh();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] ChartWidget* chart() const { return m_chart; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] int headerHeight() const;

    const core::MetricRegistry* m_registry;
    QString m_label;
    std::optional<core::MetricHandle> m_valueMetric;
    const TextRenderer* m_text;
    const Theme* m_theme;
    ChartWidget* m_chart = nullptr; // child, owned by Qt parent
};

} // namespace qk::ui
