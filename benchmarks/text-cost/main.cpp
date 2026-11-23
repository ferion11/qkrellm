// SPDX-License-Identifier: GPL-3.0-or-later
// Text-cost experiment behind ADR 0011. Standalone: not part of the main build.
//
//   cmake -S benchmarks/text-cost -B /tmp/qkrellm/build/text-cost -G Ninja
//   -DCMAKE_BUILD_TYPE=Release cmake --build /tmp/qkrellm/build/text-cost
//   B=/tmp/qkrellm/build/text-cost/text-cost
//   QT_QPA_PLATFORM=offscreen $B notext    # baseline: no text
//   QT_QPA_PLATFORM=offscreen $B system    # QPainter::drawText with system fonts
//   QT_QPA_PLATFORM=offscreen $B appfont /usr/share/fonts/dejavu/DejaVuSans.ttf
//   QT_QPA_PLATFORM=offscreen $B gen /usr/share/fonts/dejavu/DejaVuSans.ttf   # write glyph strip
//   QT_QPA_PLATFORM=offscreen $B bitmap    # blit text from the strip, no QFont at all
//
// Output files go to ${QKRELLM_OUT:-/tmp/qkrellm}/bench/text-cost/.
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGlyphRun>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRawFont>

#include <cstdio>
#include <cstring>

namespace {

QElapsedTimer g_clock;
qint64 g_lastUs = 0;

void mark(const char* what)
{
    const qint64 nowUs = g_clock.nsecsElapsed() / 1000;
    std::printf("%-24s %7.1f ms (+%6.1f)\n", what, static_cast<double>(nowUs) / 1000.0,
                static_cast<double>(nowUs - g_lastUs) / 1000.0);
    g_lastUs = nowUs;
}

void printMemory()
{
    QFile f(QStringLiteral("/proc/self/smaps_rollup"));
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    for (const QByteArray& line : f.readAll().split('\n')) {
        if (line.startsWith("Rss:") || line.startsWith("Private_Dirty:")) {
            std::printf("  %s\n", line.simplified().constData());
        }
    }
}

QString outDir()
{
    const QByteArray env = qgetenv("QKRELLM_OUT");
    const QString dir =
        (env.isEmpty() ? QStringLiteral("/tmp/qkrellm") : QString::fromLocal8Bit(env)) +
        QStringLiteral("/bench/text-cost");
    QDir().mkpath(dir);
    return dir;
}

constexpr int kFirst = 32;
constexpr int kLast = 126;
constexpr int kCell = 12;
constexpr int kHeight = 14;
constexpr int kPixelSize = 11;

int generateStrip(const QString& ttf)
{
    const QRawFont raw(ttf, kPixelSize);
    QImage strip((kLast - kFirst + 1) * kCell, kHeight, QImage::Format_ARGB32_Premultiplied);
    strip.fill(Qt::transparent);
    QFile adv(outDir() + QStringLiteral("/strip.adv"));
    if (!raw.isValid() || !adv.open(QIODevice::WriteOnly)) {
        std::fprintf(stderr, "cannot load %s or write output\n", qPrintable(ttf));
        return 1;
    }
    QPainter p(&strip);
    p.setPen(Qt::white);
    for (int c = kFirst; c <= kLast; ++c) {
        const auto glyphs = raw.glyphIndexesForString(QString(QChar(c)));
        QGlyphRun run;
        run.setRawFont(raw);
        run.setGlyphIndexes(glyphs);
        run.setPositions({QPointF(0, 0)});
        p.drawGlyphRun(QPointF((c - kFirst) * kCell, kPixelSize), run);
        adv.write(QByteArray::number(qRound(raw.advancesForGlyphIndexes(glyphs).value(0).x())) +
                  ' ');
    }
    p.end();
    strip.save(outDir() + QStringLiteral("/strip.png"));
    mark("generated strip");
    return 0;
}

int bitmapText()
{
    const QPixmap strip(outDir() + QStringLiteral("/strip.png"));
    QFile af(outDir() + QStringLiteral("/strip.adv"));
    if (strip.isNull() || !af.open(QIODevice::ReadOnly)) {
        std::fprintf(stderr, "run the 'gen' mode first\n");
        return 1;
    }
    QList<int> advances;
    for (const QByteArray& t : af.readAll().split(' ')) {
        if (!t.isEmpty()) {
            advances << t.toInt();
        }
    }
    mark("loaded strip");
    QImage img(120, 48, QImage::Format_RGB32);
    img.fill(Qt::black);
    auto draw = [&](QPainter& p, int x, int y, const char* s) {
        for (; *s != '\0'; ++s) {
            const int i = *s - kFirst;
            p.drawPixmap(x, y, strip, i * kCell, 0, kCell, kHeight);
            x += advances.value(i);
        }
    };
    {
        QPainter p(&img);
        draw(p, 2, 8, "CPU 100%");
    }
    mark("first text");
    {
        QPainter p(&img);
        draw(p, 2, 28, "MEM 3.2G eth0");
    }
    mark("second text");
    img.save(outDir() + QStringLiteral("/render-bitmap.png"));
    printMemory();
    return 0;
}

int systemText()
{
    QFont font = QApplication::font();
    font.setPixelSize(kPixelSize);
    const QFontMetrics fm(font);
    (void)fm.horizontalAdvance(QStringLiteral("CPU 100%"));
    mark("first QFontMetrics");
    QImage img(120, 48, QImage::Format_RGB32);
    img.fill(Qt::black);
    {
        QPainter p(&img);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(2, 20, QStringLiteral("CPU 100%"));
    }
    mark("first text");
    {
        QPainter p(&img);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(2, 40, QStringLiteral("MEM 3.2G eth0"));
    }
    mark("second text");
    std::printf("  family=%s\n", qPrintable(QFontInfo(font).family()));
    printMemory();
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    g_clock.start();
    const char* mode = argc > 1 ? argv[1] : "system";
    const QString arg2 = argc > 2 ? QString::fromLocal8Bit(argv[2])
                                  : QStringLiteral("/usr/share/fonts/dejavu/DejaVuSans.ttf");
    QApplication app(argc, argv);
    mark("QApplication");

    if (std::strcmp(mode, "notext") == 0) {
        printMemory();
        return 0;
    }
    if (std::strcmp(mode, "gen") == 0) {
        return generateStrip(arg2);
    }
    if (std::strcmp(mode, "bitmap") == 0) {
        return bitmapText();
    }
    if (std::strcmp(mode, "appfont") == 0) {
        const int id = QFontDatabase::addApplicationFont(arg2);
        QApplication::setFont(QFont(QFontDatabase::applicationFontFamilies(id).value(0)));
        mark("addApplicationFont");
    }
    return systemText();
}
