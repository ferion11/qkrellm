// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/netdev.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

using qk::sensors::counterRate;
using qk::sensors::NetDev;
using qk::sensors::parseNetDev;

namespace {

constexpr auto kHeader =
    "Inter-|   Receive                                                |  Transmit\n"
    " face |bytes    packets errs drop fifo frame compressed multicast|bytes"
    "    packets errs drop fifo colls carrier compressed\n";

QByteArray fixture(const char* file)
{
    QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/net/%1")
                .arg(QLatin1StringView(file)));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

std::string_view view(const QByteArray& bytes)
{
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

// NetDev is ~3 KiB: fine on the stack, but keep the pattern used for ProcStat.
std::unique_ptr<NetDev> parse(std::string_view text)
{
    auto result = parseNetDev(text);
    return result ? std::make_unique<NetDev>(*result) : nullptr;
}

std::unique_ptr<NetDev> parseBody(const std::string& body)
{
    return parse(std::string(kHeader) + body);
}

class TestNetDev final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        const QByteArray text = fixture("dev");
        QVERIFY(!text.isEmpty());
        const auto dev = parse(view(text));
        QVERIFY(dev);
        QCOMPARE(dev->count, std::size_t{3});
        QVERIFY(!dev->truncated);
        QCOMPARE(dev->interfaces[0].name(), std::string_view("lo"));
        QCOMPARE(dev->interfaces[0].rxBytes, 83553U);
        QCOMPARE(dev->interfaces[0].txPackets, 131U);
        const auto wlan = dev->find("wlan0");
        QVERIFY(wlan);
        QCOMPARE(dev->interfaces.at(*wlan).rxBytes, 1280002385U);
        QCOMPARE(dev->interfaces.at(*wlan).rxPackets, 1207274U);
        QCOMPARE(dev->interfaces.at(*wlan).txBytes, 92182284U);
        QCOMPARE(dev->interfaces.at(*wlan).txPackets, 224553U);
        QVERIFY(!dev->find("eth1"));
    }

    void ratesBetweenRealSnapshots()
    {
        const auto a = parse(view(fixture("dev")));
        const auto b = parse(view(fixture("dev.1s")));
        QVERIFY(a && b);
        const auto& wa = a->interfaces.at(a->find("wlan0").value_or(0));
        const auto& wb = b->interfaces.at(b->find("wlan0").value_or(0));
        QCOMPARE(counterRate(wa.rxBytes, wb.rxBytes, 1.0), 504.0);
        QCOMPARE(counterRate(wa.txBytes, wb.txBytes, 0.5), 864.0);
    }

    void counterRateEdgeCases()
    {
        QCOMPARE(counterRate(100, 50, 1.0), 0.0); // reset / wrap
        QCOMPARE(counterRate(0, 100, 0.0), 0.0);
        QCOMPARE(counterRate(0, 100, -1.0), 0.0);
        QCOMPARE(counterRate(0, 100, std::numeric_limits<double>::quiet_NaN()), 0.0);
        constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();
        QCOMPARE(counterRate(0, kMax, 1.0), static_cast<double>(kMax));
    }

    void rejectsWrongHeader_data()
    {
        QTest::addColumn<QByteArray>("text");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("garbage") << QByteArray("\x01\xff\n");
        QTest::newRow("stat file") << QByteArray("cpu  1 2 3 4\n");
    }
    void rejectsWrongHeader()
    {
        QFETCH(QByteArray, text);
        QVERIFY(!parse(view(text)));
    }

    void headerOnlyHasNoInterfaces()
    {
        const auto dev = parseBody("");
        QVERIFY(dev);
        QCOMPARE(dev->count, std::size_t{0});
        const auto truncatedHeader = parse("Inter-|");
        QVERIFY(truncatedHeader);
        QCOMPARE(truncatedHeader->count, std::size_t{0});
    }

    void oldKernelNoSpaceAfterColon()
    {
        const auto dev = parseBody("  eth0:4294967295 10 0 0 0 0 0 0 20 30 0 0 0 0 0 0\n");
        QVERIFY(dev);
        QCOMPARE(dev->count, std::size_t{1});
        QCOMPARE(dev->interfaces[0].name(), std::string_view("eth0"));
        QCOMPARE(dev->interfaces[0].rxBytes, 4294967295U);
        QCOMPARE(dev->interfaces[0].txBytes, 20U);
        QCOMPARE(dev->interfaces[0].txPackets, 30U);
    }

    void skipsBadLinesKeepsGoodOnes()
    {
        const auto dev = parseBody(
            "  bad1: 1 2 3\n"                                            // too few columns
            "  bad2: 1 2 3 4 5 6 7 8 x 10 11 12 13 14 15 16\n"           // garbage
            "no colon here 1 2 3 4 5 6 7 8 9 10\n"                       // no name
            "    : 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n"             // empty name
            "a/b: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n"              // invalid name
            "abcdefghijklmnop: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n" // 16 chars
            "abcdefghijklmno: 1 2 3 4 5 6 7 8 9 10\n"               // 15 chars, only 10 cols: OK
            "  ok0: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18\n" // extra cols: OK
            "  ok0: 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9 9\n"              // duplicate: ignored
            "  big: 99999999999999999999 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"); // overflow, no '\n'
        QVERIFY(dev);
        QCOMPARE(dev->count, std::size_t{2});
        QCOMPARE(dev->interfaces[0].name(), std::string_view("abcdefghijklmno"));
        QCOMPARE(dev->interfaces[0].txPackets, 10U);
        QCOMPARE(dev->interfaces[1].name(), std::string_view("ok0"));
        QCOMPARE(dev->interfaces[1].rxBytes, 1U);
        QCOMPARE(dev->interfaces[1].txBytes, 9U);
    }

    void rejectsNulInName()
    {
        // Found by fuzz_netdev: a NUL would make the stored C string empty.
        const std::string line("\0wlan0: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n", 47);
        const auto dev = parseBody(line);
        QVERIFY(dev);
        QCOMPARE(dev->count, std::size_t{0});
    }

    void truncatesBeyondMaxInterfaces()
    {
        std::string body;
        for (std::size_t i = 0; i < NetDev::kMaxInterfaces + 3; ++i) {
            body += "veth" + std::to_string(i) + ": 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n";
        }
        const auto dev = parseBody(body);
        QVERIFY(dev);
        QCOMPARE(dev->count, NetDev::kMaxInterfaces);
        QVERIFY(dev->truncated);
        QCOMPARE(dev->interfaces[NetDev::kMaxInterfaces - 1].name(), std::string_view("veth63"));
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestNetDev)
#include "tst_netdev.moc"
