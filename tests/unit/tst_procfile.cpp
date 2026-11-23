// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/procfile.h"

#include <QFile>
#include <QObject>
#include <QTemporaryFile>
#include <QTest>

#include <string_view>

using qk::sensors::ProcFile;

namespace {

class TestProcFile final : public QObject
{
    Q_OBJECT

private slots:
    void readsWholeFileRepeatedly()
    {
        const QString path =
            QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/stat");
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray expected = f.readAll();

        ProcFile file(qPrintable(path));
        QVERIFY(file.isOpen());
        for (int i = 0; i < 3; ++i) { // re-reads start at offset 0
            const auto text = file.read();
            QVERIFY(text.has_value());
            QCOMPARE(QByteArray(text->data(), static_cast<qsizetype>(text->size())), expected);
        }
    }

    void growsBufferForLargeFiles()
    {
        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        const QByteArray big(100'000, 'x');
        QCOMPARE(tmp.write(big), big.size());
        tmp.flush();

        ProcFile file(qPrintable(tmp.fileName()), 1024);
        const auto text = file.read();
        QVERIFY(text.has_value());
        QCOMPARE(text->size(), std::size_t{100'000});
        QVERIFY(file.capacity() >= 100'000);
        const std::size_t grown = file.capacity();
        QVERIFY(file.read().has_value());
        QCOMPARE(file.capacity(), grown); // steady state: no further growth
    }

    void readsRealProcStat()
    {
        ProcFile file("/proc/stat");
        QVERIFY(file.isOpen());
        const auto text = file.read();
        QVERIFY(text.has_value());
        QVERIFY(text->substr(0, 4) == "cpu ");
    }

    void missingFileIsNotOpen()
    {
        ProcFile file("/nonexistent/qkrellm/file");
        QVERIFY(!file.isOpen());
        QVERIFY(!file.read().has_value());
    }

    void movedFromIsClosed()
    {
        ProcFile a("/proc/stat");
        ProcFile b(std::move(a));
        QVERIFY(b.isOpen());
        QVERIFY(!a.isOpen()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        QVERIFY(
            !a.read().has_value()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    }
};

} // namespace

QTEST_APPLESS_MAIN(TestProcFile)
#include "tst_procfile.moc"
