// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/version.h"

#include <QObject>
#include <QTest>

namespace {

class TestVersion final : public QObject
{
    Q_OBJECT

private slots:
    void matchesProjectVersion()
    {
        QCOMPARE(qk::core::versionString(), QStringLiteral(QK_EXPECTED_VERSION));
    }
};

} // namespace

QTEST_APPLESS_MAIN(TestVersion)
#include "tst_version.moc"
