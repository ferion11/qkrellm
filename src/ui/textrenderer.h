// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QStringView>

class QColor;
class QPainter;
class QPoint;

namespace qk::ui {

/// Draws single-line panel text (ADR 0011). All values are in logical (device-independent)
/// pixels. Implementations must not allocate in draw() once warmed up.
class TextRenderer
{
public:
    TextRenderer() = default;
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;
    TextRenderer(TextRenderer&&) = delete;
    TextRenderer& operator=(TextRenderer&&) = delete;
    virtual ~TextRenderer() = default;

    /// Distance from the top of the line box to the baseline.
    [[nodiscard]] virtual int ascent() const = 0;
    /// Height of the line box.
    [[nodiscard]] virtual int lineHeight() const = 0;
    /// Width the text occupies when drawn.
    [[nodiscard]] virtual int horizontalAdvance(QStringView text) const = 0;
    /// Draws `text` with the top-left of its line box at `topLeft`.
    virtual void draw(QPainter& painter, QPoint topLeft, QStringView text,
                      const QColor& color) const = 0;
};

} // namespace qk::ui
