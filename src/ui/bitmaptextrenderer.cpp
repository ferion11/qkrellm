// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/bitmaptextrenderer.h"

#include <QColor>
#include <QFile>
#include <QPainter>

#include <string_view>

namespace qk::ui {

namespace {

constexpr qreal kHiDpiThreshold = 1.25;
constexpr std::size_t kExpectedColours = 4;

int ceilDiv(int value, int divisor)
{
    return (value + divisor - 1) / divisor;
}

} // namespace

BitmapTextRenderer::BitmapTextRenderer(const AtlasMetrics& metrics, QImage atlas)
    : m_metrics(metrics)
    , m_atlas(std::move(atlas))
{
    m_tinted.reserve(kExpectedColours);
}

std::unique_ptr<BitmapTextRenderer> BitmapTextRenderer::load(const QString& base,
                                                             qreal devicePixelRatio)
{
    const auto make = [](const QString& path) -> std::unique_ptr<BitmapTextRenderer> {
        QFile file(path + QStringLiteral(".qkatlas"));
        if (!file.open(QIODevice::ReadOnly)) {
            return nullptr;
        }
        const QByteArray text = file.readAll();
        const auto metrics = parseAtlasMetrics(
            std::string_view(text.constData(), static_cast<std::size_t>(text.size())));
        QImage atlas(path + QStringLiteral(".png"));
        if (!metrics || atlas.isNull()) {
            return nullptr;
        }
        atlas.convertTo(QImage::Format_ARGB32_Premultiplied);
        return std::unique_ptr<BitmapTextRenderer>(
            new BitmapTextRenderer(*metrics, std::move(atlas)));
    };

    if (devicePixelRatio > kHiDpiThreshold) {
        if (auto hiDpi = make(base + QStringLiteral("@2x"))) {
            return hiDpi;
        }
    }
    return make(base);
}

int BitmapTextRenderer::ascent() const
{
    return ceilDiv(m_metrics.ascent, m_metrics.scale);
}

int BitmapTextRenderer::lineHeight() const
{
    return ceilDiv(m_metrics.lineHeight, m_metrics.scale);
}

const AtlasGlyph& BitmapTextRenderer::glyphFor(char16_t unit) const
{
    if (unit < m_metrics.glyphs.size() && m_metrics.glyphs[unit].present) {
        return m_metrics.glyphs[unit];
    }
    return m_metrics.glyphs[AtlasMetrics::kFallback];
}

int BitmapTextRenderer::horizontalAdvance(QStringView text) const
{
    int deviceWidth = 0;
    for (const QChar c : text) {
        if (c.isLowSurrogate()) {
            continue; // a surrogate pair renders as a single '?'
        }
        deviceWidth += glyphFor(c.unicode()).advance;
    }
    return ceilDiv(deviceWidth, m_metrics.scale);
}

const QPixmap& BitmapTextRenderer::tinted(QRgb color) const
{
    for (const auto& [rgb, pixmap] : m_tinted) {
        if (rgb == color) {
            return pixmap;
        }
    }
    QImage image = m_atlas;
    {
        QPainter painter(&image);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(image.rect(), QColor::fromRgba(color));
    }
    return m_tinted.emplace_back(color, QPixmap::fromImage(std::move(image))).second;
}

void BitmapTextRenderer::draw(QPainter& painter, QPoint topLeft, QStringView text,
                              const QColor& color) const
{
    const QPixmap& atlas = tinted(color.rgba());
    const qreal scale = m_metrics.scale;
    // Work in atlas pixels and convert once per glyph, so 2x metrics keep their precision.
    const qreal baseline = (topLeft.y() * scale) + m_metrics.ascent;
    qreal penX = topLeft.x() * scale;
    for (const QChar c : text) {
        if (c.isLowSurrogate()) {
            continue;
        }
        const AtlasGlyph& g = glyphFor(c.unicode());
        if (g.width > 0) {
            const QRectF source(g.x, g.y, g.width, g.height);
            const QRectF target((penX + g.bearingX) / scale, (baseline - g.bearingY) / scale,
                                g.width / scale, g.height / scale);
            painter.drawPixmap(target, atlas, source);
        }
        penX += g.advance;
    }
}

} // namespace qk::ui
