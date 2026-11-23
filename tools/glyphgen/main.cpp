// SPDX-License-Identifier: GPL-3.0-or-later
// qkrellm-glyphgen — renders a TTF into a bitmap glyph atlas (PNG + .qkatlas metrics).
//
//   QT_QPA_PLATFORM=offscreen qkrellm-glyphgen <font.ttf> <pixel_size> <scale> <out_base>
//
// Writes <out_base>.png and <out_base>.qkatlas. Coverage: U+0020–U+007E, U+00A0–U+00FF.
// The format is documented in src/ui/textatlas.h; the runtime side is BitmapTextRenderer.
#include <QCryptographicHash>
#include <QFile>
#include <QGlyphRun>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QRawFont>

#include <cmath>
#include <cstdio>

namespace {

struct Rendered
{
    char32_t codepoint = 0;
    QImage image; // tight box, white on transparent
    int bearingX = 0;
    int bearingY = 0;
    int advance = 0;
    int atlasX = 0;
    int atlasY = 0;
};

constexpr int kPadding = 1;

QRect alphaBounds(const QImage& img)
{
    int left = img.width();
    int top = img.height();
    int right = -1;
    int bottom = -1;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) != 0) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    return right < 0 ? QRect() : QRect(QPoint(left, top), QPoint(right, bottom));
}

std::optional<Rendered> renderGlyph(const QRawFont& raw, char32_t codepoint)
{
    const QString text = QString::fromUcs4(&codepoint, 1);
    const QList<quint32> indexes = raw.glyphIndexesForString(text);
    if (indexes.size() != 1 || indexes.front() == 0) {
        return std::nullopt; // not in the font
    }
    const int canvas = static_cast<int>(std::ceil(raw.pixelSize())) * 4;
    const QPoint pen(canvas / 4, canvas / 2); // pen position on the baseline
    QImage scratch(canvas, canvas, QImage::Format_ARGB32_Premultiplied);
    scratch.fill(Qt::transparent);
    {
        QGlyphRun run;
        run.setRawFont(raw);
        run.setGlyphIndexes(indexes);
        run.setPositions({QPointF(0, 0)});
        QPainter painter(&scratch);
        painter.setPen(Qt::white);
        painter.drawGlyphRun(pen, run);
    }
    Rendered out;
    out.codepoint = codepoint;
    out.advance = static_cast<int>(std::lround(raw.advancesForGlyphIndexes(indexes).front().x()));
    const QRect box = alphaBounds(scratch);
    if (box.isValid()) {
        out.image = scratch.copy(box);
        out.bearingX = box.left() - pen.x();
        out.bearingY = pen.y() - box.top();
    }
    return out;
}

/// Simple shelf packing; returns the atlas size.
QSize pack(QList<Rendered>& glyphs, int maxWidth)
{
    int x = kPadding;
    int y = kPadding;
    int shelfHeight = 0;
    int usedWidth = 0;
    for (Rendered& g : glyphs) {
        if (g.image.isNull()) {
            continue;
        }
        if (x + g.image.width() + kPadding > maxWidth) {
            x = kPadding;
            y += shelfHeight + kPadding;
            shelfHeight = 0;
        }
        g.atlasX = x;
        g.atlasY = y;
        x += g.image.width() + kPadding;
        usedWidth = std::max(usedWidth, x);
        shelfHeight = std::max(shelfHeight, g.image.height());
    }
    return {usedWidth, y + shelfHeight + kPadding};
}

int fail(const char* message)
{
    std::fprintf(stderr, "qkrellm-glyphgen: %s\n", message);
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    const QGuiApplication app(argc, argv);
    const QStringList args = QGuiApplication::arguments();
    if (args.size() != 5) {
        return fail("usage: qkrellm-glyphgen <font.ttf> <pixel_size> <scale 1|2> <out_base>");
    }
    const QString& fontPath = args.at(1);
    const int pixelSize = args.at(2).toInt();
    const int scale = args.at(3).toInt();
    const QString& outBase = args.at(4);
    if (pixelSize <= 0 || (scale != 1 && scale != 2)) {
        return fail("pixel_size must be > 0 and scale 1 or 2");
    }

    QFile fontFile(fontPath);
    if (!fontFile.open(QIODevice::ReadOnly)) {
        return fail("cannot read font file");
    }
    const QByteArray fontData = fontFile.readAll();
    const QRawFont raw(fontData, pixelSize * scale, QFont::PreferFullHinting);
    if (!raw.isValid()) {
        return fail("not a usable font");
    }

    QList<Rendered> glyphs;
    const auto addRange = [&](char32_t first, char32_t last) {
        for (char32_t c = first; c <= last; ++c) {
            if (auto g = renderGlyph(raw, c)) {
                glyphs.append(std::move(*g));
            } else {
                std::fprintf(stderr, "warning: U+%04X missing from font\n",
                             static_cast<unsigned>(c));
            }
        }
    };
    addRange(U' ', U'~');
    addRange(U' ', U'ÿ');

    const QSize size = pack(glyphs, 256 * scale);
    QImage atlas(size, QImage::Format_ARGB32_Premultiplied);
    atlas.fill(Qt::transparent);
    {
        QPainter painter(&atlas);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (const Rendered& g : glyphs) {
            if (!g.image.isNull()) {
                painter.drawImage(g.atlasX, g.atlasY, g.image);
            }
        }
    }
    if (!atlas.save(outBase + QStringLiteral(".png"), "PNG", 9)) {
        return fail("cannot write PNG");
    }

    const int ascent = static_cast<int>(std::ceil(raw.ascent()));
    const int descent = static_cast<int>(std::ceil(raw.descent()));
    QFile metrics(outBase + QStringLiteral(".qkatlas"));
    if (!metrics.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail("cannot write metrics");
    }
    const QByteArray sha = QCryptographicHash::hash(fontData, QCryptographicHash::Sha256).toHex();
    QByteArray out;
    out += "# qkrellm glyph atlas — generated by tools/glyphgen, do not edit (ADR 0011)\n";
    out += "# font: " + fontPath.section(QLatin1Char('/'), -1).toUtf8() + " sha256=" + sha + '\n';
    out += "# qt: " + QByteArray(qVersion()) + " hinting=full\n";
    out += "# g <codepoint> <x> <y> <w> <h> <bearing_x> <bearing_y> <advance>\n";
    out += "qkatlas 1\n";
    out += "scale " + QByteArray::number(scale) + '\n';
    out += "pixel_size " + QByteArray::number(pixelSize) + '\n';
    out += "ascent " + QByteArray::number(ascent) + '\n';
    out += "descent " + QByteArray::number(descent) + '\n';
    out += "line_height " + QByteArray::number(ascent + descent) + '\n';
    for (const Rendered& g : glyphs) {
        out += "g " + QByteArray::number(static_cast<uint>(g.codepoint)) + ' ' +
               QByteArray::number(g.atlasX) + ' ' + QByteArray::number(g.atlasY) + ' ' +
               QByteArray::number(g.image.width()) + ' ' + QByteArray::number(g.image.height()) +
               ' ' + QByteArray::number(g.bearingX) + ' ' + QByteArray::number(g.bearingY) + ' ' +
               QByteArray::number(g.advance) + '\n';
    }
    metrics.write(out);
    std::printf("wrote %s.{png,qkatlas}: %lld glyphs, %dx%d px\n", qPrintable(outBase),
                static_cast<long long>(glyphs.size()), size.width(), size.height());
    return 0;
}
