// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/diskstats.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <memory>
#include <string>
#include <string_view>

using qk::sensors::DiskStat;
using qk::sensors::DiskStats;
using qk::sensors::parseDiskStats;

namespace {

QByteArray fixture(const char* file)
{
    QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/%1")
                .arg(QLatin1StringView(file)));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// DiskStats is ~14 KiB: keep it off the test's stack.
std::unique_ptr<DiskStats> parse(std::string_view text)
{
    return std::make_unique<DiskStats>(parseDiskStats(text));
}

std::unique_ptr<DiskStats> parse(const QByteArray& bytes)
{
    return parse(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
}

class TestDiskStats final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        const QByteArray text = fixture("diskstats");
        QVERIFY(!text.isEmpty());
        const auto s = parse(text);
        QCOMPARE(s->count, std::size_t{29}); // 16 ram + 8 loop + 2 disks + 3 partitions
        QVERIFY(!s->truncated);
        QCOMPARE(s->devices[0].name(), std::string_view("ram0"));
        const auto nvme1 = s->find("nvme1n1");
        QVERIFY(nvme1);
        const DiskStat& d = s->devices.at(*nvme1);
        QCOMPARE(d.major, 259U);
        QCOMPARE(d.minor, 0U);
        QCOMPARE(d.readsCompleted, 26050U);
        QCOMPARE(d.sectorsRead, 2246922U);
        QCOMPARE(d.writesCompleted, 153350U);
        QCOMPARE(d.sectorsWritten, 14065128U);
        QVERIFY(s->find("nvme0n1p2"));
        QVERIFY(!s->find("sda"));
    }

    void parsesSecondMachineFixture()
    {
        // Maintainer's Manjaro laptop: SATA disk + partition, optical drive, loop device.
        QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18.18-manjaro-28cpu/proc/diskstats"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const auto s = parse(f.readAll());
        QCOMPARE(s->count, std::size_t{4});
        const DiskStat& sda = s->devices[0];
        QCOMPARE(sda.name(), std::string_view("sda"));
        QCOMPARE(sda.major, 8U);
        QCOMPARE(sda.readsCompleted, 73611U);
        QCOMPARE(sda.sectorsRead, 3797170U);
        QCOMPARE(sda.writesCompleted, 55782U);
        QCOMPARE(sda.sectorsWritten, 3431032U);
        QVERIFY(s->find("sda1"));
        QVERIFY(s->find("sr0"));
        QCOMPARE(s->devices.at(s->find("loop0").value_or(0)).sectorsRead, 28U);
    }

    void snapshotsAdvance()
    {
        const auto a = parse(fixture("diskstats"));
        const auto b = parse(fixture("diskstats.1s"));
        const auto& da = a->devices.at(a->find("nvme1n1").value_or(0));
        const auto& db = b->devices.at(b->find("nvme1n1").value_or(0));
        QCOMPARE(db.sectorsWritten - da.sectorsWritten, 3832U);
    }

    void layoutsOfDifferentKernels()
    {
        // 11 fields (2.6.25+), 15 (4.18+ discard), 17 (5.5+ flush); 10 is too few.
        const auto s =
            parse(std::string_view("   8 0 sda 1 2 3 4 5 6 7 8 9 10 11\n"
                                   "   8 16 sdb 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15\n"
                                   "   8 32 sdc 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17\n"
                                   "   8 48 sdd 1 2 3 4 5 6 7 8 9 10\n"));
        QCOMPARE(s->count, std::size_t{3});
        for (std::size_t i = 0; i < 3; ++i) {
            QCOMPARE(s->devices.at(i).readsCompleted, 1U);
            QCOMPARE(s->devices.at(i).sectorsRead, 3U);
            QCOMPARE(s->devices.at(i).writesCompleted, 5U);
            QCOMPARE(s->devices.at(i).sectorsWritten, 7U);
        }
        QVERIFY(!s->find("sdd"));
    }

    void ignoresBadLines()
    {
        const std::string text =
            std::string("garbage\n"
                        "\n"
                        "8 0 sda 1 2 3 4 5 6 7 8 9 10 x\n"           // non-numeric
                        "x 0 sdb 1 2 3 4 5 6 7 8 9 10 11\n"          // bad major
                        "8 4294967296 sdc 1 2 3 4 5 6 7 8 9 10 11\n" // minor > u32
                        "8 0 a/b 1 2 3 4 5 6 7 8 9 10 11\n"          // '/'
                        "8 0 abcdefghijklmnopqrstuvwxyz012345 1 2 3 4 5 6 7 8 9 10 11\n"
                        "8 0 ok 1 2 3 4 5 6 7 8 9 10 11\n"
                        "8 0 ok 9 9 9 9 9 9 9 9 9 9 9\n" // duplicate
                        "8 0 big 1 2 99999999999999999999 4 5 6 7 8 9 10 11") +
            std::string("\n8 0 n\0l 1 2 3 4 5 6 7 8 9 10 11\n", 33);
        const auto s = parse(std::string_view(text));
        QCOMPARE(s->count, std::size_t{1});
        QCOMPARE(s->devices[0].name(), std::string_view("ok"));
        QCOMPARE(s->devices[0].sectorsRead, 3U);
    }

    void emptyAndTruncated()
    {
        QCOMPARE(parse(std::string_view())->count, std::size_t{0});
        QCOMPARE(parse(std::string_view("8 0 sda 1 2 3"))->count, std::size_t{0});
        std::string many;
        for (std::size_t i = 0; i < DiskStats::kMaxDevices + 5; ++i) {
            many +=
                "7 " + std::to_string(i) + " loop" + std::to_string(i) + " 0 0 0 0 0 0 0 0 0 0 0\n";
        }
        const auto s = parse(std::string_view(many));
        QCOMPARE(s->count, DiskStats::kMaxDevices);
        QVERIFY(s->truncated);
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestDiskStats)
#include "tst_diskstats.moc"
