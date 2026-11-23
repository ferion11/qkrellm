// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/metricregistry.h"
#include "ui/chartwidget.h"
#include "ui/textformat.h"
#include "ui/theme.h"

#include <QString>
#include <QWidget>

#include <cstdint>
#include <functional>
#include <optional>
#include <tuple>
#include <vector>

namespace qk::ui {

class TextRenderer;

/// How the header value (right side) is written.
enum class ValueFormat : std::uint8_t {
    Percent, ///< Fraction metric -> "nn%"
    Celsius, ///< temperature -> "54°C"
    Rpm,     ///< fan speed -> "2400rpm"
    Bytes,   ///< Bytes metric -> "3.2G", or "3.2G/62G" with PanelSpec::totalMetric when it fits
};

// The `{}` initializers keep GCC's -Wmissing-field-initializers quiet for designated
// initializers that omit optional members; clang-tidy calls them redundant.
// NOLINTBEGIN(readability-redundant-member-init)
struct PanelSpec
{
    QString label{};                                 ///< left side of the header, e.g. "CPU"
    std::optional<core::MetricHandle> valueMetric{}; ///< shown on the right of the header
    ValueFormat valueFormat = ValueFormat::Percent;
    std::optional<core::MetricHandle> totalMetric{}; ///< Bytes only: appended as "/total"
    std::vector<ChartSeries> series{};               ///< empty = header only
    /// Optional dynamic texts, called at every paint (must not allocate): they override
    /// `label` and the formatted value respectively. Empty return = draw nothing.
    std::function<QStringView(TextBuffer&)> labelText{};
    std::function<QStringView(TextBuffer&)> valueText{};
    ChartScale chartScale = ChartScale::Fixed;
    double chartMaximum = 1.0; ///< full height for ChartScale::Fixed
};
// NOLINTEND(readability-redundant-member-init)

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
    struct HeaderText
    {
        QStringView label;
        QStringView value;
    };
    /// Last header texts painted, to skip repaints when nothing changed.
    struct ShownText
    {
        static constexpr std::size_t kCapacity = std::tuple_size_v<TextBuffer>;
        TextBuffer chars{};
        std::size_t size = 0;
        /// Stores `text`; true if it differs from the stored one.
        bool update(QStringView text);
    };

    [[nodiscard]] HeaderText headerText(TextBuffer& labelBuffer, TextBuffer& valueBuffer) const;
    [[nodiscard]] int headerHeight() const;
    [[nodiscard]] std::optional<float> latest(std::optional<core::MetricHandle> metric) const;

    const core::MetricRegistry* m_registry;
    QString m_label;
    std::optional<core::MetricHandle> m_valueMetric;
    ValueFormat m_valueFormat;
    std::optional<core::MetricHandle> m_totalMetric;
    std::function<QStringView(TextBuffer&)> m_labelText;
    std::function<QStringView(TextBuffer&)> m_valueText;
    const TextRenderer* m_text;
    const Theme* m_theme;
    ChartWidget* m_chart = nullptr; // child, owned by Qt parent
    ShownText m_shownLabel;
    ShownText m_shownValue;
};

} // namespace qk::ui
