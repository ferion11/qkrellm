// SPDX-License-Identifier: GPL-3.0-or-later
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
#include "sensors/cpuusage.h"
#include "sensors/procstat.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <new>

// --- allocation probe ---------------------------------------------------------------------
// Replaces global operator new/delete for this test binary only, so the steady-state
// sampling path can be proven allocation-free (AGENTS.md invariant 3). Disabled under
// ASan, which supplies its own allocator.
// NOLINTBEGIN(cppcoreguidelines-macro-usage,readability-use-concise-preprocessor-directives)
#if defined(__SANITIZE_ADDRESS__)
#define QK_ALLOC_PROBE 0
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define QK_ALLOC_PROBE 0
#endif
#endif
#ifndef QK_ALLOC_PROBE
#define QK_ALLOC_PROBE 1
#endif
// NOLINTEND(cppcoreguidelines-macro-usage,readability-use-concise-preprocessor-directives)

namespace {
std::atomic<bool> g_counting{false}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<long> g_allocations{0};  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace

#if QK_ALLOC_PROBE
// NOLINTBEGIN(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,misc-new-delete-overloads)
void* operator new(std::size_t size)
{
    if (g_counting.load(std::memory_order_relaxed)) {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
    }
    if (void* p = std::malloc(size == 0 ? 1 : size)) {
        return p;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size)
{
    return ::operator new(size);
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
}
// NOLINTEND(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,misc-new-delete-overloads)
#endif
// ------------------------------------------------------------------------------------------

namespace {

using qk::core::BatteryMonitor;
using qk::core::CpuMonitor;
using qk::core::DiskMonitor;
using qk::core::FilesystemMonitor;
using qk::core::HwmonMonitor;
using qk::core::MemoryMonitor;
using qk::core::MetricRegistry;
using qk::core::MetricUnit;
using qk::core::Monitor;
using qk::core::NetMonitor;
using qk::core::Sampler;
using qk::core::SystemMonitor;

QString fixtureDir()
{
    return QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/");
}

QByteArray readFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

/// Overwrites `path` in place (same inode), as the kernel's file "changes" between reads.
bool rewrite(const QString& path, const QByteArray& content)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate) && f.write(content) == content.size();
}

class CountingMonitor final : public Monitor
{
public:
    explicit CountingMonitor(int period)
        : m_period(period)
    {}
    void sample() override { ++m_samples; }
    [[nodiscard]] int periodTicks() const override { return m_period; }
    [[nodiscard]] int samples() const { return m_samples; }

private:
    int m_period;
    int m_samples = 0;
};

class TestSampling final : public QObject
{
    Q_OBJECT

private slots:
    void registryAddFindPush()
    {
        MetricRegistry registry(4);
        const auto a = registry.add({.id = "a.busy", .label = "A", .unit = MetricUnit::Fraction});
        const auto b = registry.add({.id = "b.count", .label = "B", .unit = MetricUnit::Count});
        QCOMPARE(registry.add({.id = "a.busy", .label = "dup", .unit = MetricUnit::Count}),
                 a); // duplicate id
        QCOMPARE(registry.size(), std::size_t{2});
        QCOMPARE(registry.find("b.count").value_or(99), b);
        QVERIFY(!registry.find("missing").has_value());
        QCOMPARE(registry.info(a).label, std::string("A"));
        registry.push(a, 0.5F);
        QCOMPARE(registry.history(a).latest(), 0.5F);
        QCOMPARE(registry.history(a).capacity(), std::size_t{4});
        QVERIFY(registry.history(b).empty());
    }

    void cpuMonitorMatchesSensorMath()
    {
        const QByteArray t0 = readFile(fixtureDir() + QStringLiteral("stat"));
        const QByteArray t1 = readFile(fixtureDir() + QStringLiteral("stat.1s"));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("stat"));
        QVERIFY(rewrite(path, t0));

        MetricRegistry registry(8);
        CpuMonitor cpu(registry, qPrintable(path));
        QVERIFY(cpu.isValid());
        QCOMPARE(cpu.cpuCount(), std::size_t{20});
        QCOMPARE(registry.info(cpu.cpuBusy(19)).id, std::string("cpu19.busy"));
        QVERIFY(registry.history(cpu.busy()).empty()); // baseline only

        QVERIFY(rewrite(path, t1));
        cpu.sample();
        const auto a =
            qk::sensors::parseProcStat({t0.constData(), static_cast<std::size_t>(t0.size())});
        const auto b =
            qk::sensors::parseProcStat({t1.constData(), static_cast<std::size_t>(t1.size())});
        QVERIFY(a && b);
        const auto expected = qk::sensors::computeUsage(a->total, b->total);
        QCOMPARE(registry.history(cpu.busy()).latest(), expected.busy);
        QCOMPARE(registry.history(cpu.user()).latest(), expected.user);
        QCOMPARE(registry.history(cpu.system()).latest(), expected.system);
        const auto cpu7 = qk::sensors::computeUsage(a->cpus[7], b->cpus[7]);
        QCOMPARE(registry.history(cpu.cpuBusy(7)).latest(), cpu7.busy);
    }

    void cpuMonitorSurvivesMissingFile()
    {
        MetricRegistry registry(8);
        CpuMonitor cpu(registry, "/nonexistent/qkrellm/stat");
        QVERIFY(!cpu.isValid());
        QCOMPARE(cpu.cpuCount(), std::size_t{0});
        cpu.sample(); // must not crash or push
        QVERIFY(registry.history(cpu.busy()).empty());
    }

    void memoryMonitorReadsFixture()
    {
        MetricRegistry registry(8);
        const QString path = fixtureDir() + QStringLiteral("meminfo");
        MemoryMonitor mem(registry, qPrintable(path));
        QVERIFY(mem.isValid());
        QVERIFY(!mem.hasSwap()); // fixture machine has no swap
        // A level, not a rate: the first sample is pushed at construction.
        QCOMPARE(registry.history(mem.used()).size(), std::size_t{1});
        constexpr double kKiB = 1024.0;
        QCOMPARE(registry.history(mem.totalBytes()).latest(), static_cast<float>(65426260 * kKiB));
        QCOMPARE(registry.history(mem.usedBytes()).latest(),
                 static_cast<float>((65426260 - 59654248) * kKiB));
        QCOMPARE(registry.history(mem.used()).latest(),
                 static_cast<float>((65426260.0 - 59654248.0) / 65426260.0));
        QCOMPARE(registry.history(mem.cache()).latest(),
                 static_cast<float>((220368.0 + 5239956.0 + 117468.0) / 65426260.0));
        QCOMPARE(registry.history(mem.swapUsed()).latest(), 0.0F);
        QCOMPARE(registry.info(mem.swapUsed()).id, std::string("swap.used"));
        mem.sample();
        QCOMPARE(registry.history(mem.used()).size(), std::size_t{2});
    }

    void memoryMonitorDetectsSwap()
    {
        {
            MetricRegistry registry(8);
            MemoryMonitor fromFixture(registry, QK_FIXTURES_DIR
                                      "/proc/linux-6.18-x86_64-20cpu-swap/proc/meminfo");
            QVERIFY(fromFixture.isValid());
            QVERIFY(fromFixture.hasSwap());
            QCOMPARE(registry.history(fromFixture.swapTotalBytes()).latest(), 2097148.0F * 1024.0F);
        }
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("meminfo"));
        QVERIFY(rewrite(path, "MemTotal: 1000 kB\nMemAvailable: 750 kB\n"
                              "SwapTotal: 400 kB\nSwapFree: 300 kB\n"));
        MetricRegistry registry(8);
        MemoryMonitor mem(registry, qPrintable(path));
        QVERIFY(mem.isValid());
        QVERIFY(mem.hasSwap());
        QCOMPARE(registry.history(mem.used()).latest(), 0.25F);
        QCOMPARE(registry.history(mem.swapUsed()).latest(), 0.25F);
        QCOMPARE(registry.history(mem.swapTotalBytes()).latest(), 400.0F * 1024.0F);
    }

    void memoryMonitorSurvivesMissingFile()
    {
        MetricRegistry registry(8);
        MemoryMonitor mem(registry, "/nonexistent/qkrellm/meminfo");
        QVERIFY(!mem.isValid());
        QVERIFY(!mem.hasSwap());
        mem.sample(); // must not crash or push
        QVERIFY(registry.history(mem.used()).empty());
    }

    void netMonitorRatesFromFixtures()
    {
        const QByteArray t0 = readFile(fixtureDir() + QStringLiteral("net/dev"));
        const QByteArray t1 = readFile(fixtureDir() + QStringLiteral("net/dev.1s"));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("dev"));
        // Fake /sys/class/net: only wlan0 has a device link -> eth0 (no device), lo skipped.
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("sys/wlan0/device")));
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("sys/eth0")));
        QVERIFY(rewrite(path, t0));

        MetricRegistry registry(8);
        NetMonitor net(registry, qPrintable(path), qPrintable(dir.filePath(QStringLiteral("sys"))));
        QVERIFY(net.isValid());
        QCOMPARE(net.interfaces().size(), std::size_t{1});
        const auto& wlan = net.interfaces().front();
        QCOMPARE(wlan.name, std::string("wlan0"));
        QCOMPARE(registry.info(wlan.rx).id, std::string("net.wlan0.rx"));
        QCOMPARE(registry.info(wlan.total).unit, MetricUnit::BytesPerSecond);

        QVERIFY(rewrite(path, t1));
        const auto start = NetMonitor::Clock::now();
        // Elapsed time comes from the monotonic clock: 2 s since construction halves rates.
        net.sampleAt(start + std::chrono::seconds(2));
        const float rx = registry.history(wlan.rx).latest();
        const float tx = registry.history(wlan.tx).latest();
        QVERIFY(rx > 0.0F && rx <= 504.0F / 2.0F);
        QVERIFY(tx > 0.0F && tx <= 432.0F / 2.0F);
        QCOMPARE(registry.history(wlan.total).latest(), rx + tx);

        // Exactly one second later with identical counters: zero rates.
        net.sampleAt(start + std::chrono::seconds(3));
        QCOMPARE(registry.history(wlan.rx).latest(), 0.0F);

        // Counter reset (smaller values): 0, not a huge bogus rate.
        QVERIFY(rewrite(path, t0));
        net.sampleAt(start + std::chrono::seconds(4));
        QCOMPARE(registry.history(wlan.rx).latest(), 0.0F);
        QCOMPARE(registry.history(wlan.tx).latest(), 0.0F);
    }

    void netMonitorFallsBackToVirtualInterfaces()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("dev"));
        QVERIFY(rewrite(path, readFile(fixtureDir() + QStringLiteral("net/dev"))));
        MetricRegistry registry(8);
        // No sysfs at all (e.g. a container): every interface but lo.
        NetMonitor net(registry, qPrintable(path), "/nonexistent/qkrellm/sys");
        QCOMPARE(net.interfaces().size(), std::size_t{2});
        QCOMPARE(net.interfaces()[0].name, std::string("eth0"));
        QCOMPARE(net.interfaces()[1].name, std::string("wlan0"));
    }

    void netMonitorSurvivesMissingFile()
    {
        MetricRegistry registry(8);
        NetMonitor net(registry, "/nonexistent/qkrellm/dev");
        QVERIFY(!net.isValid());
        QVERIFY(net.interfaces().empty());
        net.sample(); // must not crash
        QCOMPARE(registry.size(), std::size_t{0});
    }

    /// Writes <dir>/block/<disk>/stat from the matching /proc/diskstats fixture line (the same
    /// fields without "major minor name"), as /sys/block/<disk>/stat presents them.
    static bool writeBlockStat(const QTemporaryDir& dir, const QByteArray& diskstats,
                               const char* disk)
    {
        for (const QByteArray& line : diskstats.split('\n')) {
            const QList<QByteArray> f = line.simplified().split(' ');
            if (f.size() > 3 && f[2] == disk) {
                return rewrite(
                    dir.filePath(QStringLiteral("block/%1/stat").arg(QLatin1StringView(disk))),
                    f.mid(3).join(' ') + '\n');
            }
        }
        return false;
    }

    void diskMonitorRatesFromFixtures()
    {
        const QByteArray t0 = readFile(fixtureDir() + QStringLiteral("diskstats"));
        const QByteArray t1 = readFile(fixtureDir() + QStringLiteral("diskstats.1s"));
        QTemporaryDir dir;
        // Fake /sys/block: both NVMe disks have a device; loop0 does not.
        for (const char* d : {"block/nvme0n1/device", "block/nvme1n1/device", "block/loop0"}) {
            QVERIFY(QDir(dir.path()).mkpath(QLatin1StringView(d)));
        }
        for (const char* d : {"nvme0n1", "nvme1n1", "loop0"}) {
            QVERIFY(writeBlockStat(dir, t0, d));
        }

        MetricRegistry registry(8);
        DiskMonitor disk(registry, dir.filePath(QStringLiteral("block")).toStdString());
        QCOMPARE(disk.disks().size(), std::size_t{2});
        QCOMPARE(disk.disks()[0].name, std::string("nvme0n1")); // sorted names
        QCOMPARE(registry.info(disk.disks()[1].write).id, std::string("disk.nvme1n1.write"));

        for (const char* d : {"nvme0n1", "nvme1n1"}) {
            QVERIFY(writeBlockStat(dir, t1, d));
        }
        const auto start = DiskMonitor::Clock::now();
        disk.sampleAt(start + std::chrono::seconds(1));
        // nvme1n1 wrote 3832 sectors, nvme0n1 read 256: rates <= the 1 s values (elapsed
        // time is measured from construction, so slightly more than 1 s).
        const float write = registry.history(disk.write()).latest();
        const float read = registry.history(disk.read()).latest();
        QVERIFY(write > 0.9F * 3832 * 512 && write <= 3832.0F * 512);
        QVERIFY(read > 0.9F * 256 * 512 && read <= 256.0F * 512);
        QCOMPARE(registry.history(disk.total()).latest(), read + write);
        QCOMPARE(registry.history(disk.disks()[1].write).latest(), write);

        for (const char* d : {"nvme0n1", "nvme1n1"}) {
            QVERIFY(writeBlockStat(dir, t0, d)); // counters went back: 0, not a huge rate
        }
        disk.sampleAt(start + std::chrono::seconds(2));
        QCOMPARE(registry.history(disk.total()).latest(), 0.0F);
    }

    void diskMonitorFallbackAndMissingTree()
    {
        const QByteArray t0 = readFile(fixtureDir() + QStringLiteral("diskstats"));
        QTemporaryDir dir;
        // No device links (e.g. some VMs): every /sys/block entry except loop/ram.
        for (const char* d : {"nvme0n1", "loop0", "ram0"}) {
            QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("block/%1").arg(QLatin1StringView(d))));
            QVERIFY(writeBlockStat(dir, t0, d));
        }
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("block/sdz"))); // no stat: skipped
        MetricRegistry registry(8);
        DiskMonitor disk(registry, dir.filePath(QStringLiteral("block")).toStdString());
        QCOMPARE(disk.disks().size(), std::size_t{1});
        QCOMPARE(disk.disks()[0].name, std::string("nvme0n1"));

        MetricRegistry other(8);
        DiskMonitor missing(other, "/nonexistent/qkrellm/block");
        QVERIFY(missing.disks().empty());
        missing.sample(); // must not crash or push
        QVERIFY(other.history(missing.total()).empty());
    }

    void filesystemMonitorFromFixture()
    {
        MetricRegistry registry(8);
        std::vector<std::string> asked;
        FilesystemMonitor fs(
            registry, QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/mounts",
            [&asked](const std::string& path) -> std::optional<qk::sensors::FsUsage> {
                asked.push_back(path);
                if (path == "/efi") {
                    return std::nullopt; // e.g. unmounted meanwhile
                }
                return qk::sensors::fsUsage(1000, 750, 700, 4096);
            });
        QCOMPARE(fs.periodTicks(), FilesystemMonitor::kPeriodTicks);
        QCOMPARE(fs.mounts().size(), std::size_t{3});
        QCOMPARE(fs.mounts()[2].mountPoint, std::string("/home"));
        QCOMPARE(registry.info(fs.mounts()[0].used).id, std::string("fs:/.used"));
        // Sampled once at construction: 250 used of 250 + 700 available.
        QCOMPARE(registry.history(fs.mounts()[0].used).latest(), static_cast<float>(250.0 / 950.0));
        QCOMPARE(registry.history(fs.mounts()[2].totalBytes).latest(), 1000.0F * 4096);
        QVERIFY(registry.history(fs.mounts()[1].used).empty()); // statvfs failed
        fs.sample();
        QCOMPARE(registry.history(fs.mounts()[0].used).size(), std::size_t{2});
        QCOMPARE(asked.size(), std::size_t{6});

        MetricRegistry other(8);
        FilesystemMonitor none(other, "/nonexistent/qkrellm/mounts");
        QVERIFY(none.mounts().empty());
        none.sample();
        QVERIFY(FilesystemMonitor::statvfs("/").has_value()); // the real one works
        QVERIFY(!FilesystemMonitor::statvfs("/nonexistent/qkrellm").has_value());
    }

    void hwmonMonitorFromFixtureTree()
    {
        MetricRegistry registry(8);
        HwmonMonitor hwmon(registry, QK_FIXTURES_DIR "/sys/linux-6.18-x86_64-20cpu/class/hwmon");
        QCOMPARE(hwmon.sensors().size(), std::size_t{22});
        QCOMPARE(registry.size(), std::size_t{0}); // nothing registered until enabled

        QCOMPARE(hwmon.cpuTemperature().value_or(99), std::size_t{7}); // coretemp Package id 0
        // CPU first, then the first temperature of each other chip, then fans.
        const std::vector<std::size_t> expected{7, 0, 1, 2, 6, 4, 5};
        QCOMPARE(hwmon.defaultSelection(), expected);
        QCOMPARE(hwmon.displayName(0), std::string("acpitz"));
        QCOMPARE(hwmon.displayName(1), std::string("nvme1")); // two "nvme" chips
        QCOMPARE(hwmon.displayName(2), std::string("nvme0"));
        QCOMPARE(hwmon.displayName(7), std::string("coretemp"));

        const auto package = hwmon.enable(7);
        QVERIFY(package);
        QCOMPARE(hwmon.enable(7), package); // idempotent
        QVERIFY(!hwmon.enable(1000));
        QCOMPARE(registry.info(*package).id, std::string("hwmon.coretemp.coretemp.0.temp1"));
        QCOMPARE(registry.info(*package).unit, MetricUnit::Celsius);
        QVERIFY(registry.history(*package).empty()); // first read happens in sample()
        QCOMPARE(hwmon.periodTicksOf(7), 0);

        const auto nvme = hwmon.enable(1);
        const auto fan = hwmon.enable(4);
        QVERIFY(nvme && fan);
        QCOMPARE(registry.info(*fan).id, std::string("hwmon.asus.asus-nb-wmi.fan1"));
        QCOMPARE(registry.info(*fan).unit, MetricUnit::Rpm);

        hwmon.sample();
        QCOMPARE(registry.history(*package).latest(), 31.0F); // 31000 millidegrees
        QCOMPARE(registry.history(*nvme).latest(), 42.85F);
        QCOMPARE(registry.history(*fan).latest(), 0.0F);
        QCOMPARE(hwmon.periodTicksOf(7), 1); // fixture files are plain files: fast
        hwmon.sample();
        QCOMPARE(registry.history(*package).size(), std::size_t{2});
        QCOMPARE(registry.size(), std::size_t{3});
    }

    void hwmonMonitorReadsSlowSensorsLessOften()
    {
        MetricRegistry registry(64);
        // Threshold 0: every sensor counts as very slow.
        HwmonMonitor hwmon(registry, QK_FIXTURES_DIR "/sys/linux-6.18-x86_64-20cpu/class/hwmon",
                           std::chrono::microseconds(0));
        const auto handle = hwmon.enable(7);
        QVERIFY(handle);
        hwmon.sample(); // tick 0: first read, timed -> very slow
        QCOMPARE(hwmon.periodTicksOf(7), HwmonMonitor::kVerySlowPeriodTicks);
        for (int i = 1; i < 2 * HwmonMonitor::kVerySlowPeriodTicks; ++i) {
            hwmon.sample();
        }
        // Ticks 0 and 30 read; 60 ticks in total.
        QCOMPARE(registry.history(*handle).size(), std::size_t{2});
    }

    void hwmonMonitorWithoutSensors()
    {
        MetricRegistry registry(8);
        HwmonMonitor hwmon(registry, "/nonexistent/qkrellm/hwmon");
        QVERIFY(hwmon.sensors().empty());
        QVERIFY(!hwmon.cpuTemperature());
        QVERIFY(hwmon.defaultSelection().empty());
        hwmon.sample(); // must not crash
    }

    void batteryMonitorFromFixtureTree()
    {
        MetricRegistry registry(8);
        BatteryMonitor battery(registry,
                               QK_FIXTURES_DIR "/sys/linux-6.18-x86_64-20cpu/class/power_supply");
        QVERIFY(battery.hasBattery());
        QCOMPARE(battery.periodTicks(), BatteryMonitor::kPeriodTicks);
        // Read at construction: values are there for the first paint.
        QCOMPARE(registry.history(battery.charge()).latest(),
                 static_cast<float>(2055000.0 / 3159000.0));
        QCOMPARE(registry.history(battery.ac()).latest(), 1.0F);
        QVERIFY(std::isnan(registry.history(battery.seconds()).latest()));
        QCOMPARE(registry.history(battery.state()).latest(), 3.0F); // Idle (Not charging)
        battery.sample();
        QCOMPARE(registry.history(battery.charge()).size(), std::size_t{2});
    }

    void batteryMonitorDischargingAndWithoutBattery()
    {
        QTemporaryDir dir;
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("BAT0")));
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("hidpp_battery_0")));
        QVERIFY(rewrite(dir.filePath(QStringLiteral("BAT0/uevent")),
                        "POWER_SUPPLY_TYPE=Battery\nPOWER_SUPPLY_STATUS=Discharging\n"
                        "POWER_SUPPLY_ENERGY_NOW=30000000\nPOWER_SUPPLY_ENERGY_FULL=60000000\n"
                        "POWER_SUPPLY_POWER_NOW=15000000\n"));
        QVERIFY(rewrite(dir.filePath(QStringLiteral("hidpp_battery_0/uevent")),
                        "POWER_SUPPLY_TYPE=Battery\nPOWER_SUPPLY_SCOPE=Device\n"
                        "POWER_SUPPLY_CAPACITY=5\n")); // a mouse: ignored
        MetricRegistry registry(8);
        BatteryMonitor battery(registry, dir.path().toStdString());
        QVERIFY(battery.hasBattery());
        QCOMPARE(registry.history(battery.charge()).latest(), 0.5F);
        QCOMPARE(registry.history(battery.seconds()).latest(), 2.0F * 3600.0F);
        QCOMPARE(registry.history(battery.ac()).latest(), 0.0F);

        MetricRegistry other(8);
        BatteryMonitor none(other, "/nonexistent/qkrellm/power_supply");
        QVERIFY(!none.hasBattery());
        none.sample(); // must not crash
        QVERIFY(other.history(none.charge()).empty());
        QCOMPARE(other.history(none.ac()).latest(), 0.0F);
    }

    void systemMonitorLoadAndUptime()
    {
        MetricRegistry registry(8);
        SystemMonitor system(registry, qPrintable(fixtureDir() + QStringLiteral("loadavg")));
        QVERIFY(system.hasLoad());
        QCOMPARE(system.periodTicks(), SystemMonitor::kPeriodTicks);
        QCOMPARE(registry.history(system.load1()).latest(), 0.63F); // read at construction
        QCOMPARE(registry.history(system.load15()).latest(), 2.27F);
        QFile uptimeFile(QStringLiteral("/proc/uptime"));
        QVERIFY(uptimeFile.open(QIODevice::ReadOnly));
        const double kernel = uptimeFile.readAll().split(' ').front().toDouble();
        QVERIFY(std::abs(static_cast<double>(registry.history(system.uptime()).latest()) - kernel) <
                5.0); // same clock as /proc/uptime
        system.sample();
        QCOMPARE(registry.history(system.load5()).size(), std::size_t{2});

        MetricRegistry other(8);
        SystemMonitor missing(other, "/nonexistent/qkrellm/loadavg");
        QVERIFY(!missing.hasLoad());
        missing.sample();
        QVERIFY(other.history(missing.load1()).empty());
        QCOMPARE(other.history(missing.uptime()).size(), std::size_t{2}); // still works
    }

    void samplerHonoursPeriodsAndSignals()
    {
        Sampler sampler(std::chrono::milliseconds(1000));
        auto every = std::make_unique<CountingMonitor>(1);
        auto third = std::make_unique<CountingMonitor>(3);
        const CountingMonitor* everyPtr = every.get();
        const CountingMonitor* thirdPtr = third.get();
        sampler.addMonitor(std::move(every));
        sampler.addMonitor(std::move(third));
        const QSignalSpy spy(&sampler, &Sampler::sampled);
        for (int i = 0; i < 7; ++i) {
            sampler.tick();
        }
        QCOMPARE(everyPtr->samples(), 7);
        QCOMPARE(thirdPtr->samples(), 2); // ticks 3 and 6
        QCOMPARE(spy.count(), 7);
        QCOMPARE(sampler.ticks(), std::uint64_t{7});
        QCOMPARE(sampler.interval(), std::chrono::milliseconds(1000));
    }

    void samplerTimerFires()
    {
        Sampler sampler(std::chrono::milliseconds(10));
        QSignalSpy spy(&sampler, &Sampler::sampled);
        sampler.start();
        QVERIFY(spy.wait(2000));
        sampler.stop();
    }

    void steadyStateSamplingDoesNotAllocate()
    {
#if !QK_ALLOC_PROBE
        QSKIP("allocation probe disabled under AddressSanitizer");
#else
        MetricRegistry registry(256);
        Sampler sampler(std::chrono::milliseconds(1000));
        auto cpu = std::make_unique<CpuMonitor>(registry, "/proc/stat"); // real kernel file
        QVERIFY(cpu->isValid());
        sampler.addMonitor(std::move(cpu));
        auto mem = std::make_unique<MemoryMonitor>(registry, "/proc/meminfo");
        QVERIFY(mem->isValid());
        sampler.addMonitor(std::move(mem));
        auto disk = std::make_unique<DiskMonitor>(registry); // real /sys/block
        sampler.addMonitor(std::move(disk));
        auto hwmon = std::make_unique<HwmonMonitor>(registry); // real /sys/class/hwmon
        for (const std::size_t i : hwmon->defaultSelection()) {
            QVERIFY(hwmon->enable(i));
        }
        sampler.addMonitor(std::move(hwmon));
        sampler.addMonitor(std::make_unique<BatteryMonitor>(registry));    // real power_supply
        sampler.addMonitor(std::make_unique<FilesystemMonitor>(registry)); // real mounts
        sampler.addMonitor(std::make_unique<SystemMonitor>(registry));     // real loadavg
        auto net = std::make_unique<NetMonitor>(registry); // real /proc/net/dev + sysfs
        QVERIFY(net->isValid());
        sampler.addMonitor(std::move(net));
        int observed = 0;
        connect(&sampler, &Sampler::sampled, this, [&observed] { ++observed; });

        sampler.tick(); // warm-up: lets any lazy one-time growth happen
        g_allocations = 0;
        g_counting = true;
        for (int i = 0; i < 50; ++i) {
            sampler.tick();
        }
        g_counting = false;
        QCOMPARE(g_allocations.load(), 0L);
        QCOMPARE(observed, 51);
        // Control: the probe really counts. The optimiser may elide a plain `new int`
        // (allowed for new-expressions), so call the replaceable function explicitly and
        // publish the pointer through a volatile.
        g_counting = true;
        void* volatile probe = ::operator new(sizeof(int));
        g_counting = false;
        ::operator delete(probe);
        QCOMPARE(g_allocations.load(), 1L);
#endif
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestSampling)
#include "tst_sampling.moc"
