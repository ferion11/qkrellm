// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/panel.h"

#include "ui/textformat.h"
#include "ui/textrenderer.h"

#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <algorithm>
#include <span>
#include <utility>

namespace qk::ui {

Panel::Panel(const core::MetricRegistry& registry, PanelSpec spec, const TextRenderer& text,
             const Theme& theme, QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_label(std::move(spec.label))
    , m_valueMetric(spec.valueMetric)
    , m_valueFormat(spec.valueFormat)
    , m_totalMetric(spec.totalMetric)
    , m_labelText(std::move(spec.labelText))
    , m_valueText(std::move(spec.valueText))
    , m_text(&text)
    , m_theme(&theme)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, headerHeight(), 0, 0);
    layout->setSpacing(0);
    if (!spec.series.empty()) {
        m_chart = new ChartWidget(registry, std::move(spec.series), theme, spec.chartScale,
                                  spec.chartMaximum, &text, this);
        layout->addWidget(m_chart);
    }
}

int Panel::headerHeight() const
{
    return m_text->lineHeight() + (2 * m_theme->panelPadding);
}

QSize Panel::sizeHint() const
{
    const int chartHeight = m_chart != nullptr ? m_chart->sizeHint().height() : 0;
    return {m_theme->panelWidth, headerHeight() + chartHeight};
}

void Panel::refresh()
{
    // Charts scroll on every sample; the header is repainted only when its text changed
    // (most header-only rows change every few seconds at most).
    if (m_chart != nullptr) {
        m_chart->refresh();
    }
    TextBuffer labelBuffer{};
    TextBuffer buffer{};
    const auto [label, value] = headerText(labelBuffer, buffer);
    const bool labelChanged = m_shownLabel.update(label); // both must run: no short-circuit
    const bool valueChanged = m_shownValue.update(value);
    if (labelChanged || valueChanged) {
        update(0, 0, width(), headerHeight());
    }
}

bool Panel::ShownText::update(QStringView text)
{
    const auto n = static_cast<std::size_t>(std::min<qsizetype>(text.size(), kCapacity));
    const std::span<const char16_t> incoming(text.utf16(), n);
    if (n == size && std::ranges::equal(incoming, std::span(chars).first(n))) {
        return false;
    }
    std::ranges::copy(incoming, chars.begin());
    size = n;
    return true;
}

Panel::HeaderText Panel::headerText(TextBuffer& labelBuffer, TextBuffer& buffer) const
{
    const int pad = m_theme->panelPadding;
    const QStringView label = m_labelText ? m_labelText(labelBuffer) : QStringView(m_label);
    QStringView value;
    if (m_valueText) {
        value = m_valueText(buffer);
    } else if (m_valueMetric) {
        switch (m_valueFormat) {
        case ValueFormat::Percent:
            value = formatPercent(buffer, latest(m_valueMetric));
            break;
        case ValueFormat::Celsius:
            value = formatCelsius(buffer, latest(m_valueMetric));
            break;
        case ValueFormat::Rpm:
            value = formatRpm(buffer, latest(m_valueMetric));
            break;
        case ValueFormat::Bytes:
            if (m_totalMetric) {
                value = formatBytesOf(buffer, latest(m_valueMetric), latest(m_totalMetric));
                // Narrow panel: "used/total" would collide with the label -> just "used".
                const int room = width() - (3 * pad) - m_text->horizontalAdvance(label);
                if (m_text->horizontalAdvance(value) > room) {
                    value = formatBytes(buffer, latest(m_valueMetric));
                }
            } else {
                value = formatBytes(buffer, latest(m_valueMetric));
            }
            break;
        }
    }
    return {.label = label, .value = value};
}

void Panel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const int header = headerHeight();
    painter.fillRect(0, 0, width(), header, m_theme->windowBackground);

    const int pad = m_theme->panelPadding;
    int labelRight = width() - pad; // the label is clipped so it never runs under the value
    TextBuffer labelBuffer{};
    TextBuffer buffer{};
    const auto [label, value] = headerText(labelBuffer, buffer);
    if (!value.isEmpty()) {
        const int valueLeft = width() - pad - m_text->horizontalAdvance(value);
        m_text->draw(painter, QPoint(valueLeft, pad), value, m_theme->text);
        labelRight = valueLeft - pad;
    }

    painter.setClipRect(0, 0, std::max(labelRight, 0), header);
    m_text->draw(painter, QPoint(pad, pad), label, m_theme->dimText);
}

std::optional<float> Panel::latest(std::optional<core::MetricHandle> metric) const
{
    if (!metric) {
        return std::nullopt;
    }
    const auto& history = m_registry->history(*metric);
    return history.empty() ? std::nullopt : std::optional(history.latest());
}

} // namespace qk::ui
