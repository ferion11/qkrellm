// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/powersupply.h"

#include <QFile>
#include <QObject>
#include <QTest>

#include <string_view>
#include <vector>

using qk::sensors::BatteryEstimate;
using qk::sensors::estimateBattery;
using qk::sensors::parsePowerSupplyUevent;
using qk::sensors::PowerSupply;

namespace {

QByteArray uevent(const char* supply)
{
    QFile f(
        QStringLiteral(QK_FIXTURES_DIR "/sys/linux-6.18-x86_64-20cpu/class/power_supply/%1/uevent")
            .arg(QLatin1StringView(supply)));
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

PowerSupply parse(const QByteArray& bytes)
{
    return parsePowerSupplyUevent({bytes.constData(), static_cast<std::size_t>(bytes.size())});
}

PowerSupply battery(PowerSupply::Status status)
{
    PowerSupply b;
    b.type = PowerSupply::Type::Battery;
    b.status = status;
    return b;
}

class TestPowerSupply final : public QObject
{
    Q_OBJECT

private slots:
    void parsesRealFixture()
    {
        const PowerSupply bat = parse(uevent("BAT1"));
        QCOMPARE(bat.type, PowerSupply::Type::Battery);
        QCOMPARE(bat.status, PowerSupply::Status::NotCharging);
        QVERIFY(bat.present.value_or(false));
        QVERIFY(!bat.deviceScope);
        QCOMPARE(bat.capacity.value_or(0), 65);
        QCOMPARE(bat.chargeNow.value_or(0), 2055000);
        QCOMPARE(bat.chargeFull.value_or(0), 3159000); // not CHARGE_FULL_DESIGN
        QCOMPARE(bat.currentNow.value_or(1), 0);
        QVERIFY(!bat.energyNow);

        const PowerSupply ac = parse(uevent("ACAD"));
        QCOMPARE(ac.type, PowerSupply::Type::Mains);
        QVERIFY(ac.online.value_or(false));

        const PowerSupply usb = parse(uevent("ucsi-source-psy-USBC000:001"));
        QCOMPARE(usb.type, PowerSupply::Type::Usb);
        QVERIFY(!usb.online.value_or(true));

        const std::vector<PowerSupply> all{ac, bat, usb};
        const BatteryEstimate e = estimateBattery(all);
        QVERIFY(e.hasBattery);
        QVERIFY(e.acOnline);
        QCOMPARE(e.state, BatteryEstimate::State::Idle);
        QCOMPARE(e.fraction.value_or(0), 2055000.0 / 3159000.0);
        QVERIFY(!e.secondsLeft); // zero current: no estimate
    }

    void toleratesJunk()
    {
        const PowerSupply p = parsePowerSupplyUevent("garbage\nPOWER_SUPPLY_CAPACITY=x\n"
                                                     "POWER_SUPPLY_=1\nPOWER_SUPPLY_TYPE\n"
                                                     "OTHER_TYPE=Battery\nPOWER_SUPPLY_PRESENT=");
        QCOMPARE(p.type, PowerSupply::Type::Unknown);
        QVERIFY(!p.capacity);
        QVERIFY(!p.present);
        QCOMPARE(parsePowerSupplyUevent("POWER_SUPPLY_TYPE=UPS").type, PowerSupply::Type::Other);
        QCOMPARE(parsePowerSupplyUevent("POWER_SUPPLY_TYPE=USB_PD").type, PowerSupply::Type::Usb);
        QCOMPARE(parsePowerSupplyUevent("POWER_SUPPLY_STATUS=Full").status,
                 PowerSupply::Status::Full);
        QVERIFY(parsePowerSupplyUevent("POWER_SUPPLY_SCOPE=Device").deviceScope);
    }

    void dischargingEnergyFamily()
    {
        PowerSupply b = battery(PowerSupply::Status::Discharging);
        b.energyNow = 30'000'000;  // 30 Wh
        b.energyFull = 60'000'000; // 60 Wh
        b.powerNow = 10'000'000;   // 10 W
        const BatteryEstimate e = estimateBattery(std::vector{b});
        QCOMPARE(e.state, BatteryEstimate::State::Discharging);
        QCOMPARE(e.fraction.value_or(0), 0.5);
        QCOMPARE(e.secondsLeft.value_or(0), 3.0 * 3600);
        QVERIFY(!e.acOnline);

        b.powerNow = -10'000'000; // some firmware: negative while discharging
        QCOMPARE(estimateBattery(std::vector{b}).secondsLeft.value_or(0), 3.0 * 3600);
    }

    void chargingChargeFamilyAndTwoBatteries()
    {
        PowerSupply a = battery(PowerSupply::Status::Charging);
        a.chargeNow = 1'000'000;
        a.chargeFull = 2'000'000;
        a.currentNow = 500'000;
        PowerSupply b = battery(PowerSupply::Status::NotCharging);
        b.chargeNow = 2'000'000;
        b.chargeFull = 2'000'000;
        b.currentNow = 0;
        const BatteryEstimate e = estimateBattery(std::vector{a, b});
        QCOMPARE(e.state, BatteryEstimate::State::Charging); // any charging wins
        QCOMPARE(e.fraction.value_or(0), 0.75);
        QCOMPARE(e.secondsLeft.value_or(0), 2.0 * 3600); // 1 Ah missing at 0.5 A
    }

    void ignoresPeripheralAndAbsentBatteries()
    {
        PowerSupply mouse = battery(PowerSupply::Status::Discharging);
        mouse.deviceScope = true;
        mouse.capacity = 10;
        PowerSupply absent = battery(PowerSupply::Status::Unknown);
        absent.present = false;
        absent.capacity = 20;
        QVERIFY(!estimateBattery(std::vector{mouse, absent}).hasBattery);
        QVERIFY(!estimateBattery(std::vector<PowerSupply>{}).hasBattery);
    }

    void capacityFallbackAndMixedFamilies()
    {
        PowerSupply a = battery(PowerSupply::Status::Discharging);
        a.energyNow = 10;
        a.energyFull = 20;
        a.powerNow = 5;
        a.capacity = 50;
        PowerSupply b = battery(PowerSupply::Status::Discharging);
        b.chargeNow = 10;
        b.chargeFull = 20;
        b.currentNow = 5;
        b.capacity = 70;
        const BatteryEstimate e = estimateBattery(std::vector{a, b});
        QCOMPARE(e.fraction.value_or(0), 0.6); // mean CAPACITY: families are never mixed
        QVERIFY(!e.secondsLeft);

        PowerSupply weird = battery(PowerSupply::Status::Discharging);
        weird.capacity = 250; // clamped
        QCOMPARE(estimateBattery(std::vector{weird}).fraction.value_or(0), 1.0);
        PowerSupply zeroFull = battery(PowerSupply::Status::Discharging);
        zeroFull.energyNow = 5;
        zeroFull.energyFull = 0;
        QVERIFY(!estimateBattery(std::vector{zeroFull}).fraction);
    }
};

} // namespace

QTEST_GUILESS_MAIN(TestPowerSupply)
#include "tst_powersupply.moc"
