// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/hwmon.h"
#include "sensors/sysfsvalue.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

#include <cstdint>
#include <limits>
#include <string>

using qk::sensors::discoverHwmon;
using qk::sensors::HwmonSensor;
using qk::sensors::parseSysfsInteger;

namespace {

const std::string kRoot = QK_FIXTURES_DIR "/sys/linux-6.18-x86_64-20cpu/class/hwmon";

class TestHwmon final : public QObject
{
    Q_OBJECT

private slots:
    void parsesSysfsIntegers()
    {
        QCOMPARE(parseSysfsInteger("42850\n").value_or(0), 42850);
        QCOMPARE(parseSysfsInteger("0").value_or(1), 0);
        QCOMPARE(parseSysfsInteger("-5000\n").value_or(0), -5000);
        QCOMPARE(parseSysfsInteger("65261850 \r\n").value_or(0), 65261850);
        QCOMPARE(parseSysfsInteger("9223372036854775807").value_or(0),
                 std::numeric_limits<std::int64_t>::max());
        QCOMPARE(parseSysfsInteger("-9223372036854775808").value_or(0),
                 std::numeric_limits<std::int64_t>::min());
        for (const char* bad :
             {"", "\n", "-", "--1", "+1", "1 2", "0x10", "abc", " 1", "9223372036854775808",
              "-9223372036854775809", "99999999999999999999"}) {
            QVERIFY2(!parseSysfsInteger(bad), bad);
        }
    }

    void discoversRealTree()
    {
        const auto sensors = discoverHwmon(kRoot);
        QCOMPARE(sensors.size(), std::size_t{22}); // 20 temperatures + 2 fans

        // Ordered by hwmon number (numerically: 1, 3, 4, 5, 7, 10), temps before fans.
        QCOMPARE(sensors[0].chip, std::string("acpitz"));
        QCOMPARE(sensors[0].label, std::string("temp1")); // no temp1_label
        QCOMPARE(sensors[0].device, std::string("thermal_zone0"));

        QCOMPARE(sensors[1].chip, std::string("nvme"));
        QCOMPARE(sensors[1].device, std::string("nvme1"));
        QCOMPARE(sensors[1].label, std::string("Composite"));
        QCOMPARE(sensors[2].device, std::string("nvme0"));
        QCOMPARE(sensors[3].label, std::string("Sensor 1"));

        QCOMPARE(sensors[4].kind, HwmonSensor::Kind::Fan);
        QCOMPARE(sensors[4].label, std::string("cpu_fan"));
        QCOMPARE(sensors[5].label, std::string("gpu_fan"));
        QCOMPARE(sensors[6].chip, std::string("iwlwifi_1"));

        const HwmonSensor& package = sensors[7];
        QCOMPARE(package.chip, std::string("coretemp"));
        QCOMPARE(package.hwmon, 10);
        QCOMPARE(package.channel, 1); // channels sorted numerically: 1, 2, 6, 10, ...
        QCOMPARE(package.label, std::string("Package id 0"));
        QCOMPARE(package.inputPath, kRoot + "/hwmon10/temp1_input");
        QCOMPARE(sensors[8].channel, 2);
        QCOMPARE(sensors[9].channel, 6);
        QCOMPARE(sensors.back().label, std::string("Core 31"));
    }

    void toleratesOddTrees()
    {
        QVERIFY(discoverHwmon("/nonexistent/qkrellm/hwmon").empty());
        QTemporaryDir dir;
        const auto write = [&](const QString& rel, const QByteArray& content) {
            QDir(dir.path()).mkpath(QFileInfo(dir.filePath(rel)).path());
            QFile f(dir.filePath(rel));
            return f.open(QIODevice::WriteOnly) && f.write(content) == content.size();
        };
        QVERIFY(write(QStringLiteral("hwmon0/temp1_input"), "1000\n")); // no "name": skipped
        QVERIFY(write(QStringLiteral("hwmonX/name"), "bad\n"));         // not hwmonN
        QVERIFY(write(QStringLiteral("hwmon1/name"), "  chip  \n"));
        QVERIFY(write(QStringLiteral("hwmon1/temp1_label"), "\n")); // empty label -> "temp1"
        QVERIFY(write(QStringLiteral("hwmon1/temp1_input"), "1000\n"));
        QVERIFY(write(QStringLiteral("hwmon1/temp_input"), "1\n"));  // no channel
        QVERIFY(write(QStringLiteral("hwmon1/tempx_input"), "1\n")); // bad channel
        QVERIFY(write(QStringLiteral("hwmon1/in0_input"), "1\n"));   // voltage: not listed
        const auto sensors = discoverHwmon(dir.path().toStdString());
        QCOMPARE(sensors.size(), std::size_t{1});
        QCOMPARE(sensors[0].chip, std::string("chip"));
        QCOMPARE(sensors[0].label, std::string("temp1"));
        QVERIFY(sensors[0].device.empty());
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestHwmon)
#include "tst_hwmon.moc"
