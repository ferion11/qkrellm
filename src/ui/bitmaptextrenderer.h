// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui/textatlas.h"
#include "ui/textrenderer.h"

#include <QImage>
#include <QPixmap>
#include <QRgb>
#include <QString>

#include <memory>
#include <vector>

namespace qk::ui {

/// Default panel text renderer: blits glyphs from a pre-rendered atlas (ADR 0011).
/// Never touches QFont, QFontMetrics or QFontDatabase, so startup and memory do not depend
/// on the fonts installed on the system. Characters outside the atlas render as '?'.
class BitmapTextRenderer final : public TextRenderer
{
public:
    /// Loads `<base>.qkatlas` + `<base>.png`, preferring `<base>@2x.*` when
    /// `devicePixelRatio` > 1.25. `base` is usually a Qt resource path such as
    /// ":/qkrellm/fonts/default". Returns nullptr if the atlas is missing or invalid.
    [[nodiscard]] static std::unique_ptr<BitmapTextRenderer> load(const QString& base,
                                                                  qreal devicePixelRatio);

    [[nodiscard]] int ascent() const override;
    [[nodiscard]] int lineHeight() const override;
    [[nodiscard]] int horizontalAdvance(QStringView text) const override;
    void draw(QPainter& painter, QPoint topLeft, QStringView text,
              const QColor& color) const override;

    /// Atlas scale actually loaded (1 or 2).
    [[nodiscard]] int scale() const { return m_metrics.scale; }

private:
    BitmapTextRenderer(const AtlasMetrics& metrics, QImage atlas);

    [[nodiscard]] const AtlasGlyph& glyphFor(char16_t unit) const;
    [[nodiscard]] const QPixmap& tinted(QRgb color) const;

    AtlasMetrics m_metrics;
    QImage m_atlas; // white glyphs, alpha = coverage
    // One tinted copy per colour used; panels use a handful of theme colours, so this
    // stays tiny and stops allocating after the first frame.
    mutable std::vector<std::pair<QRgb, QPixmap>> m_tinted;
};

} // namespace qk::ui
