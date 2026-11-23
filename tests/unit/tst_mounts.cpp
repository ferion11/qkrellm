// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/mounts.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <cstdint>
#include <limits>
#include <string>

using qk::sensors::fsUsage;
using qk::sensors::parseMounts;
using qk::sensors::selectDiskMounts;

namespace {

class TestMounts final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        QFile f(QStringLiteral(QK_FIXTURES_DIR "/proc/linux-6.18-x86_64-20cpu/proc/mounts"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray bytes = f.readAll();
        const auto mounts =
            parseMounts({bytes.constData(), static_cast<std::size_t>(bytes.size())});
        QCOMPARE(mounts.size(), std::size_t{25});
        QCOMPARE(mounts[0].device, std::string("proc"));
        QCOMPARE(mounts[6].mountPoint, std::string("/"));
        QCOMPARE(mounts[6].fsType, std::string("ext4"));

        const auto disks = selectDiskMounts(mounts);
        QCOMPARE(disks.size(), std::size_t{3});
        QCOMPARE(disks[0].mountPoint, std::string("/"));
        QCOMPARE(disks[0].device, std::string("/dev/nvme0n1p2"));
        QCOMPARE(disks[1].mountPoint, std::string("/efi"));
        QCOMPARE(disks[2].mountPoint, std::string("/home"));
    }

    void unescapesAndSkipsJunk()
    {
        const auto mounts = parseMounts("/dev/sdb1 /media/My\\040Disk ext4 rw 0 0\n"
                                        "/dev/sdc1 /mnt/back\\134slash\\011tab vfat rw 0 0\n"
                                        "/dev/sdd1 /mnt/bad\\08x\\777 ext4 rw 0 0\n" // not escapes
                                        "\n"
                                        "only two\n"
                                        "/dev/sde1 /mnt/trailing\\04 xfs");
        QCOMPARE(mounts.size(), std::size_t{4});
        QCOMPARE(mounts[0].mountPoint, std::string("/media/My Disk"));
        QCOMPARE(mounts[1].mountPoint, std::string("/mnt/back\\slash\ttab"));
        QCOMPARE(mounts[2].mountPoint, std::string("/mnt/bad\\08x\\777"));
        QCOMPARE(mounts[3].mountPoint, std::string("/mnt/trailing\\04"));
        QCOMPARE(mounts[3].fsType, std::string("xfs"));
    }

    void selectsEachDeviceOnce()
    {
        const auto disks = selectDiskMounts(parseMounts(
            "/dev/mapper/root / btrfs rw,subvol=/@ 0 0\n"
            "/dev/mapper/root /home btrfs rw,subvol=/@home 0 0\n" // same device: skipped
            "/dev/loop3 /snap/core/1 squashfs ro 0 0\n"           // snap image: skipped
            "server:/export /net nfs rw 0 0\n"                    // network: skipped
            "/dev/sda1 /boot ext4 rw 0 0\n"));
        QCOMPARE(disks.size(), std::size_t{2});
        QCOMPARE(disks[0].mountPoint, std::string("/"));
        QCOMPARE(disks[1].mountPoint, std::string("/boot"));
    }

    void computesUsageLikeDf()
    {
        // 469G ext4 (from statvfs on the reference machine, 4 KiB blocks).
        const auto u = fsUsage(122877580, 112853030, 106574500, 4096);
        QCOMPARE(u.total, std::uint64_t{122877580} * 4096);
        QCOMPARE(u.used, std::uint64_t{122877580 - 112853030} * 4096);
        // df: used / (used + avail), rounded up to 9 %.
        QVERIFY(u.fraction > 0.085 && u.fraction < 0.09);

        QCOMPARE(fsUsage(0, 0, 0, 4096).fraction, 0.0); // empty fs
        const auto full = fsUsage(100, 0, 0, 512);
        QCOMPARE(full.fraction, 1.0);
        const auto weird = fsUsage(100, 200, 300, 512); // free > blocks: clamped
        QCOMPARE(weird.used, std::uint64_t{0});
        QCOMPARE(weird.fraction, 0.0);
        constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();
        QCOMPARE(fsUsage(kMax, 0, 0, kMax).total, kMax); // saturates
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestMounts)
#include "tst_mounts.moc"
