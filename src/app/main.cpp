// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/startupprobe.h"
#include "core/metricregistry.h"
#include "core/monitors/batterymonitor.h"
#include "core/monitors/cpumonitor.h"
#include "core/monitors/diskmonitor.h"
#include "core/monitors/filesystemmonitor.h"
#include "core/monitors/hwmonmonitor.h"
#include "core/monitors/memorymonitor.h"
#include "core/monitors/netmonitor.h"
#include "core/monitors/systemmonitor.h"
#include "core/sampler.h"
#include "core/version.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <optional>

namespace {
// Enough history for a chart a few hundred pixels wide (one sample per pixel column).
constexpr std::size_t kHistoryLength = 256;
constexpr std::chrono::milliseconds kSampleInterval{1000};
constexpr double kTemperatureChartMax = 100.0; // °C at full chart height

/// CPU temperature as a chart panel, then one header-only row per other chip and fan.
void addHwmonPanels(qk::ui::MainWindow& window, qk::core::HwmonMonitor& hwmon)
{
    using qk::sensors::HwmonSensor;
    const qk::ui::Theme& theme = window.theme();
    const auto cpu = hwmon.cpuTemperature();
    for (const std::size_t i : hwmon.defaultSelection()) {
        const auto handle = hwmon.enable(i);
        if (!handle) {
            continue;
        }
        const HwmonSensor& s = hwmon.sensors()[i];
        const bool temp = s.kind == HwmonSensor::Kind::Temperature;
        qk::ui::PanelSpec spec{
            .label = QString::fromStdString(temp ? hwmon.displayName(i) : s.label),
            .valueMetric = handle,
            .valueFormat = temp ? qk::ui::ValueFormat::Celsius : qk::ui::ValueFormat::Rpm,
        };
        if (cpu && i == *cpu) {
            spec.label = QStringLiteral("Temp");
            spec.series = {{.metric = *handle, .color = theme.seriesSecondary}};
            spec.chartMaximum = kTemperatureChartMax;
        }
        window.addPanel(std::move(spec));
    }
}
std::optional<float> latestOf(const qk::core::MetricRegistry& registry, qk::core::MetricHandle h)
{
    const auto& history = registry.history(h);
    return history.empty() ? std::nullopt : std::optional(history.latest());
}

/// "Bat 65% 1:23" while on battery; the label turns into "AC" when an adapter is online.
void addBatteryPanel(qk::ui::MainWindow& window, const qk::core::MetricRegistry& registry,
                     const qk::core::BatteryMonitor& battery)
{
    const qk::core::MetricRegistry* reg = &registry;
    const auto ac = battery.ac();
    const auto charge = battery.charge();
    const auto seconds = battery.seconds();
    window.addPanel({
        .labelText = [reg, ac](qk::ui::TextBuffer&) -> QStringView {
            return latestOf(*reg, ac).value_or(0.0F) > 0.5F ? u"AC" : u"Bat";
        },
        .valueText =
            [reg, charge, seconds](qk::ui::TextBuffer& buffer) {
                const auto s = latestOf(*reg, seconds);
                return qk::ui::formatBattery(buffer, latestOf(*reg, charge),
                                             s ? std::optional<double>(*s) : std::nullopt);
            },
    });
}

std::tm localNow()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    ::localtime_r(&now, &local);
    return local;
}

/// "11-23 14:45": month-day and local time (follows the sampler, so within a second).
void addClockPanel(qk::ui::MainWindow& window)
{
    window.addPanel({
        .labelText =
            [](qk::ui::TextBuffer& buffer) {
                const std::tm t = localNow();
                return qk::ui::formatPair(buffer, t.tm_mon + 1, u'-', t.tm_mday);
            },
        .valueText =
            [](qk::ui::TextBuffer& buffer) {
                const std::tm t = localNow();
                return qk::ui::formatPair(buffer, t.tm_hour, u':', t.tm_min);
            },
    });
}

void addLoadPanel(qk::ui::MainWindow& window, const qk::core::MetricRegistry& registry,
                  qk::core::MetricHandle load)
{
    window.addPanel({
        .label = QStringLiteral("Load"),
        .valueText =
            [reg = &registry, load](qk::ui::TextBuffer& buffer) {
                return qk::ui::formatLoad(buffer, latestOf(*reg, load));
            },
    });
}

void addUptimePanel(qk::ui::MainWindow& window, const qk::core::MetricRegistry& registry,
                    qk::core::MetricHandle uptime)
{
    window.addPanel({
        .label = QStringLiteral("Up"),
        .valueText =
            [reg = &registry, uptime](qk::ui::TextBuffer& buffer) {
                return qk::ui::formatUptime(buffer, latestOf(*reg, uptime));
            },
    });
}
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
    auto disk = std::make_unique<qk::core::DiskMonitor>(registry);
    auto filesystems = std::make_unique<qk::core::FilesystemMonitor>(registry);
    auto hwmon = std::make_unique<qk::core::HwmonMonitor>(registry);
    auto battery = std::make_unique<qk::core::BatteryMonitor>(registry);
    auto net = std::make_unique<qk::core::NetMonitor>(registry);
    if (!net->isValid()) {
        qWarning("qkrellm: cannot read /proc/net/dev; network monitor disabled");
    }
    auto system = std::make_unique<qk::core::SystemMonitor>(registry);
    auto mem = std::make_unique<qk::core::MemoryMonitor>(registry);
    if (!mem->isValid()) {
        qWarning("qkrellm: cannot read /proc/meminfo; memory monitor disabled");
    }

    qk::ui::MainWindow window(registry);
    const qk::ui::Theme& theme = window.theme();
    addClockPanel(window);
    window.addPanel({
        .label = QStringLiteral("CPU"),
        .valueMetric = cpu->busy(),
        .series =
            {
                {.metric = cpu->system(), .color = theme.seriesSecondary},
                {.metric = cpu->user(), .color = theme.seriesPrimary},
            },
    });
    if (system->hasLoad()) {
        addLoadPanel(window, registry, system->load1());
    }
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
    if (!disk->disks().empty()) {
        window.addPanel({
            .label = QStringLiteral("Disk"),
            .valueMetric = disk->total(),
            .valueFormat = qk::ui::ValueFormat::Bytes,
            .series =
                {
                    {.metric = disk->read(), .color = theme.seriesPrimary},
                    {.metric = disk->write(), .color = theme.seriesSecondary},
                },
            .chartScale = qk::ui::ChartScale::Auto,
        });
    }
    for (const auto& fs : filesystems->mounts()) { // "/home 25%"
        window.addPanel({
            .label = QString::fromStdString(fs.mountPoint),
            .valueMetric = fs.used,
        });
    }
    for (const auto& itf : net->interfaces()) {
        window.addPanel({
            .label = QString::fromStdString(itf.name),
            .valueMetric = itf.total,
            .valueFormat = qk::ui::ValueFormat::Bytes,
            .series =
                {
                    {.metric = itf.rx, .color = theme.seriesPrimary},
                    {.metric = itf.tx, .color = theme.seriesSecondary},
                },
            .chartScale = qk::ui::ChartScale::Auto,
        });
    }
    addHwmonPanels(window, *hwmon);
    if (battery->hasBattery()) {
        addBatteryPanel(window, registry, *battery);
    }
    addUptimePanel(window, registry, system->uptime());
    sampler.addMonitor(std::move(cpu));
    sampler.addMonitor(std::move(mem));
    sampler.addMonitor(std::move(disk));
    sampler.addMonitor(std::move(filesystems));
    sampler.addMonitor(std::move(net));
    sampler.addMonitor(std::move(hwmon));
    sampler.addMonitor(std::move(battery));
    sampler.addMonitor(std::move(system));
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
