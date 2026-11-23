// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/metricregistry.h"
#include "fontconfigprobe.h"
#include "ui/bitmaptextrenderer.h"
#include "ui/chartwidget.h"
#include "ui/mainwindow.h"
#include "ui/panel.h"
#include "ui/textformat.h"
#include "ui/theme.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QObject>
#include <QScreen>
#include <QSignalSpy>
#include <QTest>

using qk::core::MetricRegistry;
using qk::core::MetricUnit;
using qk::ui::BitmapTextRenderer;
using qk::ui::chartAutoScale;
using qk::ui::ChartScale;
using qk::ui::ChartWidget;
using qk::ui::formatBattery;
using qk::ui::formatBytes;
using qk::ui::formatBytesOf;
using qk::ui::formatCelsius;
using qk::ui::formatDuration;
using qk::ui::formatPercent;
using qk::ui::formatRpm;
using qk::ui::MainWindow;
using qk::ui::TextBuffer;
using qk::ui::Theme;

namespace {

QString text(QStringView view)
{
    return view.toString();
}

/// Height of the run of `color` pixels ending at the bottom of column `x`.
int barHeight(const QImage& image, int x, const QColor& color)
{
    int h = 0;
    for (int y = image.height() - 1; y >= 0 && image.pixelColor(x, y) == color; --y) {
        ++h;
    }
    return h;
}

class TestPanels final : public QObject
{
    Q_OBJECT

private slots:
    void formatsPercent()
    {
        TextBuffer b{};
        QCOMPARE(text(formatPercent(b, 0.0F)), QStringLiteral("0%"));
        QCOMPARE(text(formatPercent(b, 0.054F)), QStringLiteral("5%"));
        QCOMPARE(text(formatPercent(b, 0.995F)), QStringLiteral("100%"));
        QCOMPARE(text(formatPercent(b, 1.7F)), QStringLiteral("100%"));
        QCOMPARE(text(formatPercent(b, -0.2F)), QStringLiteral("0%"));
        QCOMPARE(text(formatPercent(b, std::nullopt)), QStringLiteral("--%"));
        QCOMPARE(text(formatPercent(b, std::numeric_limits<float>::quiet_NaN())),
                 QStringLiteral("--%"));
    }

    void formatsBytes_data()
    {
        QTest::addColumn<double>("bytes");
        QTest::addColumn<QString>("expected");
        constexpr double kK = 1024.0;
        QTest::newRow("zero") << 0.0 << "0B";
        QTest::newRow("bytes") << 512.0 << "512B";
        QTest::newRow("999 B") << 999.0 << "999B";
        QTest::newRow("1000 B -> K") << 1000.0 << "1.0K";
        QTest::newRow("1.5K") << 1.5 * kK << "1.5K";
        QTest::newRow("9.96K rounds up") << 9.96 * kK << "10K";
        QTest::newRow("123M") << 123.4 * kK * kK << "123M";
        QTest::newRow("999.4M") << 999.4 * kK * kK << "999M";
        QTest::newRow("1000M -> G") << 1000.0 * kK * kK << "1.0G";
        QTest::newRow("3.2G") << 3.24 * kK * kK * kK << "3.2G";
        QTest::newRow("62G") << 65426260.0 * kK << "62G";
        QTest::newRow("2T") << 2.0 * kK * kK * kK * kK << "2.0T";
        QTest::newRow("16 EiB (uint64 max)") << 18446744073709551615.0 << "16E";
        QTest::newRow("beyond: capped") << 1e30 << "999E";
    }
    void formatsBytes()
    {
        QFETCH(double, bytes);
        QFETCH(QString, expected);
        TextBuffer b{};
        QCOMPARE(text(formatBytes(b, bytes)), expected);
    }

    void formatsMissingBytes()
    {
        TextBuffer b{};
        QCOMPARE(text(formatBytes(b, std::nullopt)), QStringLiteral("--"));
        QCOMPARE(text(formatBytes(b, -1.0)), QStringLiteral("--"));
        QCOMPARE(text(formatBytes(b, std::numeric_limits<double>::quiet_NaN())),
                 QStringLiteral("--"));
        QCOMPARE(text(formatBytesOf(b, 5.5 * 1024 * 1024 * 1024, 62.0 * 1024 * 1024 * 1024)),
                 QStringLiteral("5.5G/62G"));
        QCOMPARE(text(formatBytesOf(b, std::nullopt, 1024.0)), QStringLiteral("--/1.0K"));
    }

    void formatsTemperatureAndFan()
    {
        TextBuffer b{};
        QCOMPARE(text(formatCelsius(b, 54.4F)), QStringLiteral("54\u00B0C"));
        QCOMPARE(text(formatCelsius(b, 42.85F)), QStringLiteral("43\u00B0C"));
        QCOMPARE(text(formatCelsius(b, -4.6F)), QStringLiteral("-5\u00B0C"));
        QCOMPARE(text(formatCelsius(b, 0.0F)), QStringLiteral("0\u00B0C"));
        QCOMPARE(text(formatCelsius(b, 1e30F)), QStringLiteral("9999\u00B0C"));
        QCOMPARE(text(formatCelsius(b, std::nullopt)), QStringLiteral("--\u00B0C"));
        QCOMPARE(text(formatCelsius(b, std::numeric_limits<float>::quiet_NaN())),
                 QStringLiteral("--\u00B0C"));
        QCOMPARE(text(formatRpm(b, 2400.4F)), QStringLiteral("2400rpm"));
        QCOMPARE(text(formatRpm(b, -1.0F)), QStringLiteral("0rpm"));
        QCOMPARE(text(formatRpm(b, std::nullopt)), QStringLiteral("--rpm"));
    }

    void formatsDurationAndBattery()
    {
        TextBuffer b{};
        QCOMPARE(text(formatDuration(b, 0.0)), QStringLiteral("0:00"));
        QCOMPARE(text(formatDuration(b, 3900.0)), QStringLiteral("1:05"));
        QCOMPARE(text(formatDuration(b, 3929.0)), QStringLiteral("1:05")); // rounds to minute
        QCOMPARE(text(formatDuration(b, 3931.0)), QStringLiteral("1:06"));
        QCOMPARE(text(formatDuration(b, 45000.0)), QStringLiteral("12:30"));
        QCOMPARE(text(formatDuration(b, 1e12)), QStringLiteral("99:59"));
        QCOMPARE(text(formatDuration(b, -1.0)), QStringLiteral("--:--"));
        QCOMPARE(text(formatDuration(b, std::nullopt)), QStringLiteral("--:--"));
        QCOMPARE(text(formatBattery(b, 0.65F, std::nullopt)), QStringLiteral("65%"));
        QCOMPARE(text(formatBattery(b, 0.65F, 4980.0)), QStringLiteral("65% 1:23"));
        QCOMPARE(text(formatBattery(b, 0.65F, std::numeric_limits<double>::quiet_NaN())),
                 QStringLiteral("65%"));
        QCOMPARE(text(formatBattery(b, std::nullopt, std::nullopt)), QStringLiteral("--%"));
    }

    void dynamicLabelAndValue()
    {
        MetricRegistry registry(8);
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        const Theme theme;
        bool ac = false;
        int labelCalls = 0;
        qk::ui::Panel panel(
            registry,
            {
                .labelText = [&](TextBuffer&) -> QStringView {
                    ++labelCalls;
                    return ac ? u"AC" : u"Bat";
                },
                .valueText =
                    [](TextBuffer& buffer) { return formatBattery(buffer, 0.65F, 4980.0); },
            },
            *r, theme);
        panel.resize(theme.panelWidth, panel.sizeHint().height());
        const QImage bat = panel.grab().toImage();
        ac = true;
        const QImage onAc = panel.grab().toImage();
        QCOMPARE(labelCalls, 2);
        QVERIFY(bat != onAc); // the label changed
        // The value "65% 1:23" is right-aligned and fits beside "Bat".
        QVERIFY(r->horizontalAdvance(u"Bat") + r->horizontalAdvance(u"65% 1:23") +
                    (3 * theme.panelPadding) <=
                theme.panelWidth);
    }

    void fixedScaleChartUsesGivenMaximum()
    {
        Theme theme;
        theme.chartHeight = 40;
        MetricRegistry registry(8);
        const auto t = registry.add({.id = "t", .label = "T", .unit = MetricUnit::Celsius});
        registry.push(t, 50.0F);
        registry.push(t, 150.0F); // above the maximum: clipped to full height
        ChartWidget chart(registry, {{.metric = t, .color = Qt::red}}, theme, ChartScale::Fixed,
                          100.0);
        chart.resize(2, theme.chartHeight);
        const QImage image = chart.grab().toImage();
        QCOMPARE(chart.lastScale(), 100.0);
        QCOMPARE(barHeight(image, 1, Qt::red), 40);
        QCOMPARE(barHeight(image, 0, Qt::red), 20);
    }

    void headersFitDefaultPanel()
    {
        // In the default (narrow) width, at both atlas scales:
        //  - every label fits next to its worst-case short value ("100%", "999M"), and
        //  - the usual memory header "Mem" + "7.1G/62G" still fits in full.
        for (const qreal dpr : {1.0, 2.0}) {
            const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), dpr);
            QVERIFY(r);
            const Theme theme;
            const auto fits = [&](QStringView label, QStringView value) {
                const int used = r->horizontalAdvance(label) + r->horizontalAdvance(value) +
                                 (3 * theme.panelPadding);
                return used <= theme.panelWidth;
            };
            QVERIFY(fits(u"CPU", u"100%"));
            QVERIFY(fits(u"Swap", u"999M"));
            QVERIFY(fits(u"Mem", u"7.1G/62G"));
            QVERIFY(fits(u"Temp", u"100\u00B0C"));
            QVERIFY(fits(u"/home", u"100%"));
            QVERIFY(fits(u"iwlwifi_1", u"45\u00B0C"));
        }
    }

    void bytesHeaderFallsBackToUsedWhenNarrow()
    {
        MetricRegistry registry(8);
        const auto used = registry.add({.id = "u", .label = "U", .unit = MetricUnit::Bytes});
        const auto total = registry.add({.id = "t", .label = "T", .unit = MetricUnit::Bytes});
        registry.push(used, 999.0F * 1024 * 1024);
        registry.push(total, 1000.0F * 1024 * 1024 * 1024);
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        const Theme theme;
        const auto render = [&](int width) {
            qk::ui::Panel panel(registry,
                                {
                                    .label = QStringLiteral("Swap"),
                                    .valueMetric = used,
                                    .valueFormat = qk::ui::ValueFormat::Bytes,
                                    .totalMetric = total,
                                },
                                *r, theme);
            panel.resize(width, panel.sizeHint().height());
            return panel.grab().toImage();
        };
        // Leftmost lit column of the value text, scanning from the right edge inwards
        // until a gap wider than a space separates value from label.
        const auto valueLeft = [&](const QImage& image) {
            int left = image.width();
            int gap = 0;
            for (int x = image.width() - 1; x >= 0 && gap < 6; --x) {
                bool lit = false;
                for (int y = 0; y < image.height() && !lit; ++y) {
                    lit = image.pixelColor(x, y).lightness() > 128;
                }
                gap = lit ? 0 : gap + 1;
                left = lit ? x : left;
            }
            return left;
        };
        const int wide = 200;
        const QImage full = render(wide);
        const QImage narrow = render(theme.panelWidth);
        // Wide: the whole "999M/1.0T" is drawn; narrow: only "999M".
        QVERIFY(wide - valueLeft(full) > r->horizontalAdvance(u"999M/") + theme.panelPadding);
        QVERIFY(theme.panelWidth - valueLeft(narrow) <=
                r->horizontalAdvance(u"999M") + theme.panelPadding);
    }

    void chartStacksSeriesNewestAtRight()
    {
        Theme theme;
        theme.chartHeight = 40;
        MetricRegistry registry(64);
        const auto system = registry.add({.id = "s", .label = "S", .unit = MetricUnit::Fraction});
        const auto user = registry.add({.id = "u", .label = "U", .unit = MetricUnit::Fraction});
        // Oldest -> newest: (0.25, 0.25), (0.5, 0.5) -> stacked newest column is full height.
        registry.push(system, 0.25F);
        registry.push(user, 0.25F);
        registry.push(system, 0.5F);
        registry.push(user, 0.5F);

        ChartWidget chart(
            registry, {{.metric = system, .color = Qt::red}, {.metric = user, .color = Qt::green}},
            theme);
        chart.resize(10, theme.chartHeight);
        const QImage image = chart.grab().toImage();

        // Newest column (x = 9): 20 px red at the bottom, 20 px green above it.
        QCOMPARE(barHeight(image, 9, Qt::red), 20);
        QCOMPARE(image.pixelColor(9, 0), QColor(Qt::green));
        // Previous column (x = 8): 10 px red, then 10 px green, then background.
        QCOMPARE(barHeight(image, 8, Qt::red), 10);
        QCOMPARE(image.pixelColor(8, 40 - 15), QColor(Qt::green));
        QCOMPARE(image.pixelColor(8, 40 - 25), theme.chartBackground);
        // No data further left: background (or the 50 % guide line).
        QCOMPARE(image.pixelColor(7, 39), theme.chartBackground);
    }

    void autoScaleIsPowerOfTwoCeiling()
    {
        QCOMPARE(chartAutoScale(0.0), 1024.0); // floor: 1 KiB
        QCOMPARE(chartAutoScale(-5.0), 1024.0);
        QCOMPARE(chartAutoScale(std::numeric_limits<double>::quiet_NaN()), 1024.0);
        QCOMPARE(chartAutoScale(1024.0), 1024.0);
        QCOMPARE(chartAutoScale(1025.0), 2048.0);
        QCOMPARE(chartAutoScale(3.0 * 1024 * 1024), 4.0 * 1024 * 1024);
        QVERIFY(std::isfinite(chartAutoScale(std::numeric_limits<double>::infinity())));
    }

    void autoScaledChartUsesVisibleStackedMaximum()
    {
        Theme theme;
        theme.chartHeight = 32;
        MetricRegistry registry(16);
        const auto rx =
            registry.add({.id = "rx", .label = "rx", .unit = MetricUnit::BytesPerSecond});
        const auto tx =
            registry.add({.id = "tx", .label = "tx", .unit = MetricUnit::BytesPerSecond});
        // Oldest -> newest. Stacked maxima: 6000 (old, scrolled out of a 2-column chart),
        // then 1500 and 3000 -> visible max 3000 -> scale 4096 (not 8192).
        registry.push(rx, 5000.0F);
        registry.push(tx, 1000.0F);
        registry.push(rx, 1000.0F);
        registry.push(tx, 500.0F);
        registry.push(rx, 2048.0F);
        registry.push(tx, 952.0F);
        ChartWidget chart(registry,
                          {{.metric = rx, .color = Qt::red}, {.metric = tx, .color = Qt::green}},
                          theme, ChartScale::Auto);
        chart.resize(2, theme.chartHeight);
        const QImage image = chart.grab().toImage();
        QCOMPARE(chart.lastScale(), 4096.0);
        QCOMPARE(barHeight(image, 1, Qt::red), 16); // 2048 / 4096 of 32 px
        QCOMPARE(image.pixelColor(1, 32 - 16 - 1), QColor(Qt::green));
        QCOMPARE(barHeight(image, 0, Qt::red), 8); // 1000 / 4096 * 32 = 7.8 -> 8

        chart.resize(3, theme.chartHeight); // the 6000 peak is visible again
        (void)chart.grab();
        QCOMPARE(chart.lastScale(), 8192.0);
    }

    void longLabelIsClippedBeforeValue()
    {
        MetricRegistry registry(8);
        const auto rate = registry.add({.id = "r", .label = "r", .unit = MetricUnit::Bytes});
        registry.push(rate, 999.0F * 1024 * 1024);
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        const Theme theme;
        qk::ui::Panel panel(registry,
                            {
                                .label = QStringLiteral("enp0s20f0u1u2"),
                                .valueMetric = rate,
                                .valueFormat = qk::ui::ValueFormat::Bytes,
                            },
                            *r, theme);
        panel.resize(theme.panelWidth, panel.sizeHint().height());
        const QImage image = panel.grab().toImage();
        // A clear column of background separates the clipped label from the value.
        const int valueLeft = theme.panelWidth - theme.panelPadding - r->horizontalAdvance(u"999M");
        for (int x = valueLeft - theme.panelPadding; x < valueLeft; ++x) {
            for (int y = 0; y < image.height(); ++y) {
                QCOMPARE(image.pixelColor(x, y), theme.windowBackground);
            }
        }
    }

    void chartRefreshSkipsIdenticalFrames()
    {
        // Counts paint events reaching the chart.
        struct PaintCounter final : QObject
        {
            int paints = 0;
            bool eventFilter(QObject* /*watched*/, QEvent* event) override
            {
                paints += event->type() == QEvent::Paint ? 1 : 0;
                return false;
            }
        };
        Theme theme;
        MetricRegistry registry(16);
        const auto m = registry.add({.id = "m", .label = "M", .unit = MetricUnit::Fraction});
        for (int i = 0; i < 4; ++i) {
            registry.push(m, 0.5F);
        }
        ChartWidget chart(registry, {{.metric = m, .color = Qt::red}}, theme);
        chart.resize(8, theme.chartHeight);
        PaintCounter counter;
        chart.installEventFilter(&counter);
        chart.show();
        QVERIFY(QTest::qWaitForWindowExposed(&chart));
        chart.refresh(); // first refresh: content key changes from "nothing"
        QTRY_VERIFY(counter.paints > 0);
        QCoreApplication::processEvents();
        const int before = counter.paints;

        registry.push(m, 0.5F); // a fifth column appears: different frame -> repaint
        chart.refresh();
        QCoreApplication::processEvents();
        QVERIFY(counter.paints > before);
        const int afterGrowth = counter.paints;

        for (int i = 0; i < 4; ++i) { // fill the 8 visible columns with the same value
            registry.push(m, 0.5F);
            chart.refresh();
        }
        QCoreApplication::processEvents();
        const int full = counter.paints;
        registry.push(m, 0.5F); // all 8 visible columns already equal: identical frame
        chart.refresh();
        QCoreApplication::processEvents();
        QCOMPARE(counter.paints, full);
        registry.push(m, 0.25F); // a different newest value: repaint
        chart.refresh();
        QCoreApplication::processEvents();
        QVERIFY(counter.paints > full);
        QVERIFY(afterGrowth >= before);
    }

    void scrolledChartMatchesFullRepaint()
    {
        // The on-screen pixels after many scroll-based refreshes must equal a full render.
        const auto r = BitmapTextRenderer::load(QStringLiteral(":/qkrellm/fonts/default"), 1.0);
        QVERIFY(r);
        Theme theme;
        MetricRegistry registry(64);
        const auto rx = registry.add({.id = "rx", .label = "rx", .unit = MetricUnit::Bytes});
        const auto tx = registry.add({.id = "tx", .label = "tx", .unit = MetricUnit::Bytes});
        ChartWidget chart(registry,
                          {{.metric = rx, .color = Qt::red}, {.metric = tx, .color = Qt::green}},
                          theme, ChartScale::Auto, 1.0, r.get());
        chart.resize(24, theme.chartHeight);
        chart.show();
        QVERIFY(QTest::qWaitForWindowExposed(&chart));
        QScreen* screen = chart.screen();
        QVERIFY(screen != nullptr);
        for (int i = 0; i < 40; ++i) {
            // Varying values, one scale change (peak at i = 20) and long flat stretches.
            const float v = i == 20 ? 9000.0F : static_cast<float>((i % 7) * 100);
            registry.push(rx, v);
            registry.push(tx, i < 30 ? 50.0F : 0.0F);
            chart.refresh();
            QCoreApplication::processEvents();
            const QImage shown =
                screen->grabWindow(chart.winId()).toImage().convertToFormat(QImage::Format_RGB32);
            const QImage full = chart.grab().toImage().convertToFormat(QImage::Format_RGB32);
            if (shown.size() != full.size()) {
                QSKIP("this platform cannot grab the window's backing store");
            }
            QVERIFY2(shown == full, qPrintable(QStringLiteral("frame %1 differs").arg(i)));
        }
    }

    void chartClipsStackAtTop()
    {
        Theme theme;
        MetricRegistry registry(8);
        const auto a = registry.add({.id = "a", .label = "A", .unit = MetricUnit::Fraction});
        const auto b = registry.add({.id = "b", .label = "B", .unit = MetricUnit::Fraction});
        registry.push(a, 0.8F);
        registry.push(b, 0.8F); // sum 1.6: must clip, not draw above the widget
        ChartWidget chart(
            registry, {{.metric = a, .color = Qt::red}, {.metric = b, .color = Qt::green}}, theme);
        chart.resize(4, theme.chartHeight);
        const QImage image = chart.grab().toImage();
        const int red = barHeight(image, 3, Qt::red);
        QCOMPARE(red, qRound(0.8 * theme.chartHeight));
        QCOMPARE(image.pixelColor(3, 0), QColor(Qt::green));
    }

    void windowRendersPanelWithoutFontDatabase()
    {
        MetricRegistry registry(64);
        const auto busy =
            registry.add({.id = "cpu.busy", .label = "CPU", .unit = MetricUnit::Fraction});
        registry.push(busy, 0.42F);
        MainWindow window(registry);
        auto* panel = window.addPanel({
            .label = QStringLiteral("CPU"),
            .valueMetric = busy,
            .series = {{.metric = busy, .color = Qt::cyan}},
        });
        QVERIFY(panel->chart() != nullptr);
        const QSignalSpy painted(&window, &MainWindow::firstPaintDone);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = window.grab().toImage();
        QCOMPARE(image.width(), window.theme().panelWidth + 2);
        // Header text was drawn (some light pixels in the header band).
        int light = 0;
        for (int x = 0; x < image.width(); ++x) {
            for (int y = 0; y < 14; ++y) {
                light += image.pixelColor(x, y).lightness() > 128 ? 1 : 0;
            }
        }
        QVERIFY(light > 20);
        window.refresh();
        QTRY_VERIFY(painted.count() == 1);
        QCOMPARE(qk::test::fcFontListCalls.load(), 0); // ADR 0011: no font database
    }

    // Must stay last: proves the probe is live in this executable (else the check above
    // would pass vacuously). Populates the font database for the rest of the process.
    void control_probeObservesSystemFonts()
    {
        const QFontMetrics metrics{QFont()};
        QVERIFY(metrics.horizontalAdvance(QStringLiteral("CPU")) > 0);
        if (qk::test::fcFontListCalls.load() == 0) {
            QSKIP("Qt does not use fontconfig here; the font-database probe cannot observe it");
        }
    }
};

} // namespace

QTEST_MAIN(TestPanels)
#include "tst_panels.moc"
