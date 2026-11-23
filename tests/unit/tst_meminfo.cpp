// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/meminfo.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <cstdint>
#include <string_view>

using qk::sensors::computeMemoryUsage;
using qk::sensors::MemInfo;
using qk::sensors::parseMemInfo;

namespace {

constexpr std::uint64_t kKiB = 1024;

QByteArray fixture(const char* tag)
{
    QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/%1/proc/meminfo").arg(QLatin1StringView(tag)));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

std::string_view view(const QByteArray& bytes)
{
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

class TestMemInfo final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixtureWithoutSwap()
    {
        const QByteArray text = fixture("linux-6.18-x86_64-20cpu");
        QVERIFY(!text.isEmpty());
        const auto info = parseMemInfo(view(text));
        QVERIFY(info);
        QCOMPARE(info->memTotal.value_or(0), 65426260 * kKiB);
        QCOMPARE(info->memFree.value_or(0), 56752372 * kKiB);
        QCOMPARE(info->memAvailable.value_or(0), 59654248 * kKiB);
        QCOMPARE(info->buffers.value_or(0), 220368 * kKiB);
        QCOMPARE(info->cached.value_or(0), 5239956 * kKiB); // not "SwapCached"
        QCOMPARE(info->sReclaimable.value_or(0), 117468 * kKiB);
        QCOMPARE(info->swapTotal.value_or(1), std::uint64_t{0});
        QCOMPARE(info->swapFree.value_or(1), std::uint64_t{0});

        const auto usage = computeMemoryUsage(*info);
        QCOMPARE(usage.total, 65426260 * kKiB);
        QCOMPARE(usage.used, (65426260 - 59654248) * kKiB);
        QCOMPARE(usage.cache, (220368 + 5239956 + 117468) * kKiB);
        QCOMPARE(usage.swapTotal, std::uint64_t{0});
        QCOMPARE(usage.swapUsed, std::uint64_t{0});
    }

    void parsesRealFixtureWithSwap()
    {
        // Same machine with a 2 GiB swap file enabled (unused at capture time).
        const QByteArray text = fixture("linux-6.18-x86_64-20cpu-swap");
        QVERIFY(!text.isEmpty());
        const auto info = parseMemInfo(view(text));
        QVERIFY(info);
        QCOMPARE(info->swapTotal.value_or(0), 2097148 * kKiB);
        QCOMPARE(info->swapFree.value_or(0), 2097148 * kKiB);
        const auto usage = computeMemoryUsage(*info);
        QCOMPARE(usage.total, 65426260 * kKiB);
        QCOMPARE(usage.used, (65426260 - 57851376) * kKiB);
        QCOMPARE(usage.cache, (225484 + 5431756 + 119776) * kKiB);
        QCOMPARE(usage.swapTotal, 2097148 * kKiB);
        QCOMPARE(usage.swapUsed, std::uint64_t{0});
    }

    void rejectsInputWithoutMemTotal_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("garbage") << QByteArray("\x01\xff\nnot meminfo\n");
        QTest::newRow("no MemTotal") << QByteArray("MemFree: 10 kB\nSwapTotal: 0 kB\n");
        QTest::newRow("zero") << QByteArray("MemTotal: 0 kB\n");
        QTest::newRow("no unit") << QByteArray("MemTotal: 1024\n");
        QTest::newRow("other unit") << QByteArray("MemTotal: 1024 MB\n");
        QTest::newRow("trailing junk") << QByteArray("MemTotal: 1024 kB x\n");
        QTest::newRow("negative") << QByteArray("MemTotal: -1 kB\n");
        QTest::newRow("truncated") << QByteArray("MemTotal: ");
        QTest::newRow("overflow on *1024") << QByteArray("MemTotal: 18014398509481984 kB\n");
        QTest::newRow("overflow uint64") << QByteArray("MemTotal: 99999999999999999999 kB\n");
    }
    void rejectsInputWithoutMemTotal()
    {
        QFETCH(QByteArray, text);
        QVERIFY(!parseMemInfo(view(text)));
    }

    void largestRepresentableValue()
    {
        // 2^54 - 1 kB is the largest value that still fits in bytes.
        const auto info = parseMemInfo("MemTotal: 18014398509481983 kB");
        QVERIFY(info);
        QCOMPARE(info->memTotal.value_or(0), 18014398509481983ULL * kKiB);
    }

    void toleratesUnknownReorderedAndMalformedLines()
    {
        const auto info = parseMemInfo("Future:    7 kB\n"
                                       "SwapFree:  oops kB\n"
                                       "HugePages_Total:       0\n"
                                       "MemTotal:\t 1000 kB\n"
                                       "MemTotal:  5 kB\n"   // later duplicate ignored
                                       "Cached:    100 kB"); // no final newline
        QVERIFY(info);
        QCOMPARE(info->memTotal.value_or(0), 1000 * kKiB);
        QCOMPARE(info->cached.value_or(0), 100 * kKiB);
        QVERIFY(!info->swapFree);
        QVERIFY(!info->memAvailable);
    }

    void oldKernelWithoutMemAvailable()
    {
        // Pre-3.14 layout: used = total - free - buffers - cached - SReclaimable.
        const auto info = parseMemInfo("MemTotal: 1000 kB\nMemFree: 200 kB\nBuffers: 50 kB\n"
                                       "Cached: 300 kB\nSReclaimable: 50 kB\n");
        QVERIFY(info);
        const auto usage = computeMemoryUsage(*info);
        QCOMPARE(usage.used, 400 * kKiB);
        QCOMPARE(usage.cache, 400 * kKiB);
    }

    void swapUsage()
    {
        const auto info = parseMemInfo("MemTotal: 1000 kB\nMemAvailable: 600 kB\n"
                                       "SwapTotal: 2048 kB\nSwapFree: 512 kB\n");
        QVERIFY(info);
        const auto usage = computeMemoryUsage(*info);
        QCOMPARE(usage.used, 400 * kKiB);
        QCOMPARE(usage.cache, std::uint64_t{0}); // no cache lines at all
        QCOMPARE(usage.swapTotal, 2048 * kKiB);
        QCOMPARE(usage.swapUsed, 1536 * kKiB);
    }

    void inconsistentNumbersSaturate()
    {
        // Kernel counters are not read atomically: cross-field relations can be violated.
        const auto info = parseMemInfo("MemTotal: 1000 kB\nMemAvailable: 1200 kB\n"
                                       "Cached: 5000 kB\nSwapTotal: 10 kB\nSwapFree: 20 kB\n");
        QVERIFY(info);
        const auto usage = computeMemoryUsage(*info);
        QCOMPARE(usage.used, std::uint64_t{0});
        QCOMPARE(usage.cache, 1000 * kKiB); // capped so used + cache <= total
        QCOMPARE(usage.swapUsed, std::uint64_t{0});

        MemInfo old;
        old.memTotal = 1000 * kKiB;
        old.memFree = 900 * kKiB;
        old.cached = 500 * kKiB;
        const auto u = computeMemoryUsage(old);
        QCOMPARE(u.used, std::uint64_t{0});
        QVERIFY(u.used + u.cache <= u.total);
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestMemInfo)
#include "tst_meminfo.moc"
