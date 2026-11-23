// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui/textrenderer.h"

#include <QFont>
#include <QFontMetrics>

namespace qk::ui {

/// Opt-in renderer using system fonts through QPainter::drawText (ADR 0011).
/// Constructing it populates Qt's font database: cost grows with the number of fonts
/// installed. Also used as the fallback when a bitmap atlas cannot be loaded.
class SystemFontTextRenderer final : public TextRenderer
{
public:
    explicit SystemFontTextRenderer(const QFont& font);

    [[nodiscard]] int ascent() const override;
    [[nodiscard]] int lineHeight() const override;
    [[nodiscard]] int horizontalAdvance(QStringView text) const override;
    void draw(QPainter& painter, QPoint topLeft, QStringView text,
              const QColor& color) const override;

private:
    QFont m_font;
    QFontMetrics m_metrics;
};

} // namespace qk::ui
