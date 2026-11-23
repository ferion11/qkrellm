// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/textatlas.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <string>
#include <string_view>

using qk::ui::parseAtlasMetrics;

namespace {

constexpr std::string_view kMinimal = "# comment\n"
                                      "\n"
                                      "qkatlas 1\n"
                                      "scale 1\n"
                                      "pixel_size 11\n"
                                      "ascent 11\n"
                                      "descent 3\n"
                                      "line_height 14\n"
                                      "g 63 10 20 5 8 -1 8 6\n"
                                      "g 32 0 0 0 0 0 0 4\n";

std::string withLine(std::string_view base, std::string_view line)
{
    return std::string(base) + std::string(line) + "\n";
}

class TestTextAtlas final : public QObject
{
    Q_OBJECT

private slots:
    void parsesMinimalAtlas()
    {
        const auto m = parseAtlasMetrics(kMinimal);
        QVERIFY(m.has_value());
        QCOMPARE(m->scale, 1);
        QCOMPARE(m->pixelSize, 11);
        QCOMPARE(m->ascent, 11);
        QCOMPARE(m->descent, 3);
        QCOMPARE(m->lineHeight, 14);
        const auto& q = m->glyphs[U'?'];
        QVERIFY(q.present);
        QCOMPARE(q.x, 10);
        QCOMPARE(q.y, 20);
        QCOMPARE(q.width, 5);
        QCOMPARE(q.height, 8);
        QCOMPARE(q.bearingX, -1);
        QCOMPARE(q.bearingY, 8);
        QCOMPARE(q.advance, 6);
        QVERIFY(m->glyphs[U' '].present);
        QVERIFY(!m->glyphs[U'A'].present);
    }

    void toleratesCrLfAndUnknownKeys()
    {
        std::string crlf;
        for (const char c : kMinimal) {
            if (c == '\n') {
                crlf += '\r';
            }
            crlf += c;
        }
        QVERIFY(parseAtlasMetrics(crlf).has_value());
        QVERIFY(parseAtlasMetrics(withLine(kMinimal, "future_key 42 whatever")).has_value());
    }

    void rejectsInvalidInput_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("no header") << QByteArray("scale 1\npixel_size 11\nline_height 14\n"
                                                 "g 63 0 0 1 1 0 0 1\n");
        QTest::newRow("wrong version") << QByteArray(
            std::string("qkatlas 2") + std::string(kMinimal.substr(kMinimal.find("\nscale"))));
        QTest::newRow("bad scale") << QByteArray(withLine(kMinimal, "scale 3").c_str());
        QTest::newRow("codepoint out of range")
            << QByteArray(withLine(kMinimal, "g 256 0 0 1 1 0 0 1").c_str());
        QTest::newRow("negative size")
            << QByteArray(withLine(kMinimal, "g 65 0 0 -1 1 0 0 1").c_str());
        QTest::newRow("garbage number")
            << QByteArray(withLine(kMinimal, "g 65 0 0 1x 1 0 0 1").c_str());
        QTest::newRow("truncated glyph") << QByteArray(withLine(kMinimal, "g 65 0 0").c_str());
        QTest::newRow("huge number")
            << QByteArray(withLine(kMinimal, "ascent 99999999999").c_str());
        QTest::newRow("missing fallback glyph")
            << QByteArray("qkatlas 1\nscale 1\npixel_size 11\nline_height 14\n"
                          "g 65 0 0 1 1 0 0 1\n");
    }

    void rejectsInvalidInput()
    {
        QFETCH(QByteArray, text);
        QVERIFY(!parseAtlasMetrics(
                     std::string_view(text.constData(), static_cast<std::size_t>(text.size())))
                     .has_value());
    }

    void shippedAtlasesParse_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<int>("scale");
        QTest::newRow("1x") << QStringLiteral(QK_SOURCE_DIR "/src/ui/fonts/default.qkatlas") << 1;
        QTest::newRow("2x") << QStringLiteral(QK_SOURCE_DIR "/src/ui/fonts/default@2x.qkatlas")
                            << 2;
    }

    void shippedAtlasesParse()
    {
        QFETCH(QString, file);
        QFETCH(int, scale);
        QFile f(file);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray text = f.readAll();
        const auto m = parseAtlasMetrics(
            std::string_view(text.constData(), static_cast<std::size_t>(text.size())));
        QVERIFY(m.has_value());
        QCOMPARE(m->scale, scale);
        for (char32_t c = 0x20; c <= 0x7e; ++c) {
            QVERIFY2(m->glyphs[c].present, qPrintable(QString::number(c, 16)));
        }
        for (char32_t c = 0xa0; c <= 0xff; ++c) {
            QVERIFY2(m->glyphs[c].present, qPrintable(QString::number(c, 16)));
        }
    }
};

} // namespace

QTEST_APPLESS_MAIN(TestTextAtlas)
#include "tst_textatlas.moc"
