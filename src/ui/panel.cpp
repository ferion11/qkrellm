// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/panel.h"

#include "ui/textformat.h"
#include "ui/textrenderer.h"

#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <utility>

namespace qk::ui {

Panel::Panel(const core::MetricRegistry& registry, PanelSpec spec, const TextRenderer& text,
             const Theme& theme, QWidget* parent)
    : QWidget(parent)
    , m_registry(&registry)
    , m_label(std::move(spec.label))
    , m_valueMetric(spec.valueMetric)
    , m_text(&text)
    , m_theme(&theme)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, headerHeight(), 0, 0);
    layout->setSpacing(0);
    if (!spec.series.empty()) {
        m_chart = new ChartWidget(registry, std::move(spec.series), theme, this);
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
    update(); // header area; children are refreshed by Qt as part of the same update
    if (m_chart != nullptr) {
        m_chart->update();
    }
}

void Panel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    const int header = headerHeight();
    painter.fillRect(0, 0, width(), header, m_theme->windowBackground);

    const int pad = m_theme->panelPadding;
    m_text->draw(painter, QPoint(pad, pad), m_label, m_theme->dimText);

    if (m_valueMetric) {
        const auto& history = m_registry->history(*m_valueMetric);
        TextBuffer buffer{};
        const QStringView value =
            formatPercent(buffer, history.empty() ? std::nullopt : std::optional(history.latest()));
        m_text->draw(painter, QPoint(width() - pad - m_text->horizontalAdvance(value), pad), value,
                     m_theme->text);
    }
}

} // namespace qk::ui
