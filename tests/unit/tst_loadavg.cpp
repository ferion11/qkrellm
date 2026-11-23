// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/loadavg.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <string_view>

using qk::sensors::parseLoadAvg;

namespace {

class TestLoadAvg final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/loadavg"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray bytes = f.readAll();
        const auto load = parseLoadAvg({bytes.constData(), static_cast<std::size_t>(bytes.size())});
        QVERIFY(load);
        QCOMPARE(load->one, 0.63);
        QCOMPARE(load->five, 0.67);
        QCOMPARE(load->fifteen, 2.27);
    }

    void acceptsAndRejects()
    {
        QCOMPARE(parseLoadAvg("1 2 3")->fifteen, 3.0);
        QCOMPARE(parseLoadAvg("1234.05 0.00 10.5 1/2 3\n")->one, 1234.05);
        for (const char* bad : {
                 "",
                 "\n",
                 "1 2",
                 "a b c",
                 "-1 2 3",
                 "1. 2 3",
                 ".5 2 3",
                 "1.2.3 2 3",
                 "1e3 2 3",
                 "nan 2 3",
                 "0,5 1 1",
                 "1.0123456789 2 3",
                 "99999999999999999999 2 3",
             }) {
            QVERIFY2(!parseLoadAvg(bad), bad);
        }
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestLoadAvg)
#include "tst_loadavg.moc"
