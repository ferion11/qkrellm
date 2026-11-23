// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/metricregistry.h"
#include "fontconfigprobe.h"
#include "ui/chartwidget.h"
#include "ui/mainwindow.h"
#include "ui/panel.h"
#include "ui/textformat.h"
#include "ui/theme.h"

#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

using qk::core::MetricRegistry;
using qk::core::MetricUnit;
using qk::ui::ChartWidget;
using qk::ui::formatPercent;
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
