// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/startupprobe.h"
#include "core/metricregistry.h"
#include "core/monitors/cpumonitor.h"
#include "core/monitors/memorymonitor.h"
#include "core/sampler.h"
#include "core/version.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <chrono>
#include <cstdio>
#include <memory>

namespace {
// Enough history for a chart a few hundred pixels wide (one sample per pixel column).
constexpr std::size_t kHistoryLength = 256;
constexpr std::chrono::milliseconds kSampleInterval{1000};
} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qkrellm"));
    QApplication::setApplicationVersion(qk::core::versionString());
    QApplication::setDesktopFileName(QStringLiteral("io.github.ferion11.qkrellm"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Lightweight Qt6 system monitor"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption exitAfterFirstPaint(
        QStringLiteral("exit-after-first-paint"),
        QStringLiteral("Print 'first-paint-ms=<n>' and quit after the first paint (benchmarks)."));
    parser.addOption(exitAfterFirstPaint);
    parser.process(app);

    qk::core::MetricRegistry registry(kHistoryLength);
    qk::core::Sampler sampler(kSampleInterval);
    auto cpu = std::make_unique<qk::core::CpuMonitor>(registry);
    if (!cpu->isValid()) {
        qWarning("qkrellm: cannot read /proc/stat; CPU monitor disabled");
    }
    auto mem = std::make_unique<qk::core::MemoryMonitor>(registry);
    if (!mem->isValid()) {
        qWarning("qkrellm: cannot read /proc/meminfo; memory monitor disabled");
    }

    qk::ui::MainWindow window(registry);
    const qk::ui::Theme& theme = window.theme();
    window.addPanel({
        .label = QStringLiteral("CPU"),
        .valueMetric = cpu->busy(),
        .series =
            {
                {.metric = cpu->system(), .color = theme.seriesSecondary},
                {.metric = cpu->user(), .color = theme.seriesPrimary},
            },
    });
    window.addPanel({
        .label = QStringLiteral("Mem"),
        .valueMetric = mem->usedBytes(),
        .valueFormat = qk::ui::ValueFormat::Bytes,
        .totalMetric = mem->totalBytes(),
        .series =
            {
                {.metric = mem->used(), .color = theme.seriesPrimary},
                {.metric = mem->cache(), .color = theme.seriesMuted},
            },
    });
    if (mem->hasSwap()) {
        window.addPanel({
            .label = QStringLiteral("Swap"),
            .valueMetric = mem->swapUsedBytes(),
            .valueFormat = qk::ui::ValueFormat::Bytes,
            .totalMetric = mem->swapTotalBytes(),
            .series = {{.metric = mem->swapUsed(), .color = theme.seriesSecondary}},
        });
    }
    sampler.addMonitor(std::move(cpu));
    sampler.addMonitor(std::move(mem));
    QObject::connect(&sampler, &qk::core::Sampler::sampled, &window, &qk::ui::MainWindow::refresh);

    if (parser.isSet(exitAfterFirstPaint)) {
        QObject::connect(&window, &qk::ui::MainWindow::firstPaintDone, &app, [] {
            // Read the clock from a queued call: by then the whole frame (children included)
            // has been painted and flushed.
            QTimer::singleShot(0, qApp, [] {
                const auto ms = qk::app::msSinceProcessStart();
                std::printf("first-paint-ms=%lld\n", ms.value_or(-1));
                std::fflush(stdout);
                QCoreApplication::quit();
            });
        });
    }

    window.show();
    sampler.start();
    return QApplication::exec();
}
