// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/cpuusage.h"
#include "sensors/procstat.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <memory>
#include <string>
#include <string_view>

using qk::sensors::computeUsage;
using qk::sensors::CpuTimes;
using qk::sensors::parseProcStat;
using qk::sensors::ProcStat;

namespace {

constexpr auto kTag = "linux-6.18-x86_64-20cpu";

QByteArray fixture(const char* tag, const char* file)
{
    QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/%1/proc/%2")
                .arg(QLatin1StringView(tag), QLatin1StringView(file)));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

std::string_view view(const QByteArray& bytes)
{
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

// ProcStat is ~20 KiB: keep it off the test's stack.
std::unique_ptr<ProcStat> parse(std::string_view text)
{
    auto result = parseProcStat(text);
    return result ? std::make_unique<ProcStat>(*result) : nullptr;
}

class TestProcStat final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        const QByteArray text = fixture(kTag, "stat");
        QVERIFY(!text.isEmpty());
        const auto s = parse(view(text));
        QVERIFY(s);
        QCOMPARE(s->total.fields, 10);
        QCOMPARE(s->total.user, 548883U);
        QCOMPARE(s->total.idle, 10071159U);
        QCOMPARE(s->cpuSlots, 20);
        QVERIFY(!s->cpusTruncated);
        for (int i = 0; i < s->cpuSlots; ++i) {
            QVERIFY(s->cpus.at(static_cast<std::size_t>(i)).present());
        }
        QCOMPARE(s->cpus[19].user, 23684U);
        QCOMPARE(s->contextSwitches.value_or(0), 58861375U);
        QCOMPARE(s->bootTime.value_or(0), 1795424945U);
        QCOMPARE(s->procsRunning.value_or(99), 2U);
        QCOMPARE(s->procsBlocked.value_or(99), 0U);
    }

    void usageBetweenRealSnapshots()
    {
        const QByteArray t0 = fixture(kTag, "stat");
        const QByteArray t1 = fixture(kTag, "stat.1s");
        const auto a = parse(view(t0));
        const auto b = parse(view(t1));
        QVERIFY(a && b);
        const auto u = computeUsage(a->total, b->total);
        QVERIFY(u.busy > 0.0F && u.busy < 1.0F);
        QVERIFY(u.user + u.system + u.iowait + u.steal <= 1.0F + 1e-6F);
        // 20 CPUs over ~1 s at USER_HZ=100 is roughly 2000 ticks.
        const auto elapsed = b->total.total() - a->total.total();
        QVERIFY2(elapsed > 1500 && elapsed < 3000, qPrintable(QString::number(elapsed)));
    }

    void olderKernelColumnCounts_data()
    {
        QTest::addColumn<QByteArray>("line");
        QTest::addColumn<int>("fields");
        QTest::newRow("2.4: 4 columns") << QByteArray("cpu 1 2 3 4\n") << 4;
        QTest::newRow("2.6.0: 7 columns") << QByteArray("cpu 1 2 3 4 5 6 7\n") << 7;
        QTest::newRow("2.6.11: 8 columns") << QByteArray("cpu 1 2 3 4 5 6 7 8\n") << 8;
        QTest::newRow("2.6.24: 9 columns") << QByteArray("cpu 1 2 3 4 5 6 7 8 9\n") << 9;
        QTest::newRow("future: 12 columns") << QByteArray("cpu 1 2 3 4 5 6 7 8 9 10 11 12\n") << 10;
    }

    void olderKernelColumnCounts()
    {
        QFETCH(QByteArray, line);
        QFETCH(int, fields);
        const auto s = parse(view(line));
        QVERIFY(s);
        QCOMPARE(s->total.fields, fields);
        QCOMPARE(s->total.idle, 4U);
        QCOMPARE(s->total.steal, fields >= 8 ? 8U : 0U);
        QCOMPARE(s->cpuSlots, 0);
    }

    void offlineCpuGapsKeepIndices()
    {
        const auto s = parse("cpu  9 0 0 9\ncpu0 1 0 0 1\ncpu3 2 0 0 2\n");
        QVERIFY(s);
        QCOMPARE(s->cpuSlots, 4);
        QVERIFY(s->cpus[0].present());
        QVERIFY(!s->cpus[1].present());
        QVERIFY(!s->cpus[2].present());
        QCOMPARE(s->cpus[3].user, 2U);
    }

    void tooManyCpusAreTruncated()
    {
        const auto s = parse("cpu 1 1 1 1\ncpu0 1 1 1 1\ncpu256 1 1 1 1\ncpu99999999999 1 1 1 1\n");
        QVERIFY(s);
        QCOMPARE(s->cpuSlots, 1);
        QVERIFY(s->cpusTruncated);
    }

    void rejectsBrokenAggregateLine_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("no aggregate line") << QByteArray("cpu0 1 2 3 4\nctxt 5\n");
        QTest::newRow("too few columns") << QByteArray("cpu 1 2 3\n");
        QTest::newRow("non numeric") << QByteArray("cpu 1 2 x 4\n");
        QTest::newRow("negative") << QByteArray("cpu 1 2 -3 4\n");
        QTest::newRow("uint64 overflow") << QByteArray("cpu 1 2 3 18446744073709551616\n");
    }

    void rejectsBrokenAggregateLine()
    {
        QFETCH(QByteArray, text);
        QVERIFY(!parse(view(text)));
    }

    void toleratesNoiseAndTruncation()
    {
        // Largest uint64 is accepted; a bad per-CPU line and a truncated tail are skipped.
        const auto s = parse("garbage line\n"
                             "cpu 18446744073709551615 0 0 1\n"
                             "cpu0 1 2 x 4\n"
                             "cpu1 1 2 3 4\n"
                             "ctxt notanumber\n"
                             "procs_running 3\n"
                             "cpu2 5 6");
        QVERIFY(s);
        QCOMPARE(s->total.user, 18446744073709551615U);
        QVERIFY(!s->cpus[0].present());
        QVERIFY(s->cpus[1].present());
        QVERIFY(!s->cpus[2].present()); // "cpu2 5 6" has too few columns
        QVERIFY(!s->contextSwitches.has_value());
        QCOMPARE(s->procsRunning.value_or(0), 3U);
    }

    void usageClampsBackwardsCounters()
    {
        CpuTimes before;
        before.user = 100;
        before.idle = 100;
        before.iowait = 50; // iowait can go backwards on some kernels
        before.fields = 10;
        CpuTimes after = before;
        after.user = 150;
        after.idle = 150;
        after.iowait = 40;
        const auto u = computeUsage(before, after);
        QCOMPARE(u.iowait, 0.0F);
        QCOMPARE(u.busy, 0.5F);
        QCOMPARE(u.user, 0.5F);
    }

    void usageWithoutElapsedTimeIsZero()
    {
        CpuTimes t;
        t.user = 5;
        t.fields = 4;
        const auto u = computeUsage(t, t);
        QCOMPARE(u.busy, 0.0F);
        // A whole CPU going backwards (hotplug) must not produce garbage either.
        CpuTimes reset;
        reset.fields = 4;
        QCOMPARE(computeUsage(t, reset).busy, 0.0F);
    }
};

} // namespace

QTEST_APPLESS_MAIN(TestProcStat)
#include "tst_procstat.moc"
