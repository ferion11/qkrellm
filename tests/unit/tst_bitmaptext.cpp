// SPDX-License-Identifier: GPL-3.0-or-later
// Verifies ADR 0011: panel text renders without populating Qt's font database.
#include "ui/bitmaptextrenderer.h"
#include "ui/systemfonttextrenderer.h"

#include <QImage>
#include <QObject>
#include <QPainter>
#include <QTest>

#include <atomic>

#include <dlfcn.h>

// --- fontconfig probe --------------------------------------------------------------------
// Qt populates its font database with FcFontList(). This executable is linked with
// ENABLE_EXPORTS, so this definition interposes the one in libfontconfig and counts calls.
// Opaque pointer types stand in for FcFontSet/FcConfig/FcPattern/FcObjectSet.
namespace {
std::atomic<int> g_fcFontListCalls{0}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
using FcFontListFn = void* (*)(void*, void*, void*);
} // namespace

extern "C" __attribute__((visibility("default"))) void* FcFontList(void* config, void* pattern,
                                                                   void* objectSet)
{
    g_fcFontListCalls.fetch_add(1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): dlsym returns void*
    static const auto real = reinterpret_cast<FcFontListFn>(dlsym(RTLD_NEXT, "FcFontList"));
    return real != nullptr ? real(config, pattern, objectSet) : nullptr;
}
// ------------------------------------------------------------------------------------------

namespace {

using qk::ui::BitmapTextRenderer;
using qk::ui::SystemFontTextRenderer;

int inkPixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) != qRgb(0, 0, 0)) {
                ++count;
            }
        }
    }
    return count;
}

QImage render(const qk::ui::TextRenderer& renderer, QStringView text, qreal dpr = 1.0)
{
    QImage image(QSize(160, 32) * dpr, QImage::Format_RGB32);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::black);
    QPainter painter(&image);
    renderer.draw(painter, QPoint(2, 2), text, Qt::white);
    return image;
}

class TestBitmapText final : public QObject
{
    Q_OBJECT

private slots:
    void loadsBothScales()
    {
        const auto normal =
            BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(normal);
        QCOMPARE(normal->scale(), 1);
        const auto hiDpi = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 2.0);
        QVERIFY(hiDpi);
        QCOMPARE(hiDpi->scale(), 2);
        // Logical metrics must be close across scales so layouts don't jump on HiDPI.
        QVERIFY(qAbs(normal->lineHeight() - hiDpi->lineHeight()) <= 1);
        QVERIFY(qAbs(normal->horizontalAdvance(u"CPU 100%") -
                     hiDpi->horizontalAdvance(u"CPU 100%")) <= 3);
    }

    void missingAtlasReturnsNull()
    {
        QVERIFY(!BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/nope"), 1.0));
    }

    void rendersInk()
    {
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        QCOMPARE(inkPixels(render(*r, u"")), 0);
        QVERIFY(inkPixels(render(*r, u"CPU 100% MEM 3.2G")) > 50);
        QVERIFY(inkPixels(render(*r, u"Memória Ação", 2.0)) > 50);
    }

    void unsupportedCharactersUseFallback()
    {
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        QCOMPARE(r->horizontalAdvance(u"€"), r->horizontalAdvance(u"?"));
        // U+1F600 is a surrogate pair: one fallback glyph, not two.
        QCOMPARE(r->horizontalAdvance(u"\U0001F600"), r->horizontalAdvance(u"?"));
        QCOMPARE(render(*r, u"€"), render(*r, u"?"));
    }

    void advanceIsAdditive()
    {
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        QCOMPARE(r->horizontalAdvance(u"ab"),
                 r->horizontalAdvance(u"a") + r->horizontalAdvance(u"b"));
    }

    void bitmapPathNeverPopulatesFontDatabase()
    {
        // Everything above ran through the bitmap path only.
        QCOMPARE(g_fcFontListCalls.load(), 0);
    }

    // Must stay last: it populates the font database for the rest of the process.
    void control_systemFontsArePopulatedAndObserved()
    {
        const SystemFontTextRenderer system{QFont()};
        QVERIFY(inkPixels(render(system, u"CPU 100%")) > 0);
        if (g_fcFontListCalls.load() == 0) {
            QSKIP("Qt does not use fontconfig here; the font-database probe cannot observe it");
        }
        QVERIFY(g_fcFontListCalls.load() > 0);
    }
};

} // namespace

QTEST_MAIN(TestBitmapText)
#include "tst_bitmaptext.moc"
