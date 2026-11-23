// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/systemfonttextrenderer.h"

#include <QColor>
#include <QPainter>

namespace qk::ui {

SystemFontTextRenderer::SystemFontTextRenderer(const QFont& font)
    : m_font(font)
    , m_metrics(font)
{}

int SystemFontTextRenderer::ascent() const
{
    return m_metrics.ascent();
}

int SystemFontTextRenderer::lineHeight() const
{
    return m_metrics.height();
}

int SystemFontTextRenderer::horizontalAdvance(QStringView text) const
{
    return m_metrics.horizontalAdvance(text.toString());
}

void SystemFontTextRenderer::draw(QPainter& painter, QPoint topLeft, QStringView text,
                                  const QColor& color) const
{
    painter.setFont(m_font);
    painter.setPen(color);
    // QPainter has no QStringView overload; this opt-in path accepts the copy.
    painter.drawText(QPoint(topLeft.x(), topLeft.y() + m_metrics.ascent()), text.toString());
}

} // namespace qk::ui
