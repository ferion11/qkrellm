// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/powersupply.h"

#include "sensors/sysfsvalue.h"
#include "sensors/textscan.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace qk::sensors {

namespace {

constexpr double kSecondsPerHour = 3600.0;
constexpr double kPercent = 100.0;

PowerSupply::Type parseType(std::string_view v)
{
    if (v == "Battery") {
        return PowerSupply::Type::Battery;
    }
    if (v == "Mains") {
        return PowerSupply::Type::Mains;
    }
    if (v.starts_with("USB")) {
        return PowerSupply::Type::Usb;
    }
    return v.empty() ? PowerSupply::Type::Unknown : PowerSupply::Type::Other;
}

PowerSupply::Status parseStatus(std::string_view v)
{
    if (v == "Charging") {
        return PowerSupply::Status::Charging;
    }
    if (v == "Discharging") {
        return PowerSupply::Status::Discharging;
    }
    if (v == "Not charging") {
        return PowerSupply::Status::NotCharging;
    }
    if (v == "Full") {
        return PowerSupply::Status::Full;
    }
    return PowerSupply::Status::Unknown;
}

std::optional<bool> parseFlag(std::string_view v)
{
    const auto n = parseSysfsInteger(v);
    if (!n) {
        return std::nullopt;
    }
    return *n != 0;
}

/// Sums `field` over `batteries`; nullopt unless every battery has it.
template<typename Field>
std::optional<double> sumAll(std::span<const PowerSupply* const> batteries, Field field)
{
    double sum = 0.0;
    for (const PowerSupply* b : batteries) {
        const std::optional<std::int64_t>& v = b->*field;
        if (!v) {
            return std::nullopt;
        }
        sum += static_cast<double>(*v);
    }
    return batteries.empty() ? std::nullopt : std::optional(sum);
}

} // namespace

PowerSupply parsePowerSupplyUevent(std::string_view text)
{
    PowerSupply ps;
    while (!text.empty()) {
        const std::string_view line = nextLine(text);
        const auto eq = line.find('=');
        if (eq == std::string_view::npos || !line.starts_with("POWER_SUPPLY_")) {
            continue;
        }
        const std::string_view key = line.substr(13, eq - 13);
        const std::string_view value = line.substr(eq + 1);
        const auto number = [&] { return parseSysfsInteger(value); };
        if (key == "TYPE") {
            ps.type = parseType(value);
        } else if (key == "STATUS") {
            ps.status = parseStatus(value);
        } else if (key == "SCOPE") {
            ps.deviceScope = value == "Device";
        } else if (key == "PRESENT") {
            ps.present = parseFlag(value);
        } else if (key == "ONLINE") {
            ps.online = parseFlag(value);
        } else if (key == "CAPACITY") {
            ps.capacity = number();
        } else if (key == "ENERGY_NOW") {
            ps.energyNow = number();
        } else if (key == "ENERGY_FULL") {
            ps.energyFull = number();
        } else if (key == "POWER_NOW") {
            ps.powerNow = number();
        } else if (key == "CHARGE_NOW") {
            ps.chargeNow = number();
        } else if (key == "CHARGE_FULL") {
            ps.chargeFull = number();
        } else if (key == "CURRENT_NOW") {
            ps.currentNow = number();
        }
    }
    return ps;
}

BatteryEstimate estimateBattery(std::span<const PowerSupply> supplies)
{
    BatteryEstimate e;
    constexpr std::size_t kMaxBatteries = 8;
    std::array<const PowerSupply*, kMaxBatteries> storage{};
    std::size_t count = 0;
    for (const PowerSupply& s : supplies) {
        if ((s.type == PowerSupply::Type::Mains || s.type == PowerSupply::Type::Usb) &&
            s.online.value_or(false)) {
            e.acOnline = true;
        }
        if (s.type == PowerSupply::Type::Battery && !s.deviceScope && s.present.value_or(true) &&
            count < kMaxBatteries) {
            storage.at(count++) = &s;
        }
    }
    const std::span<const PowerSupply* const> batteries(storage.data(), count);
    if (batteries.empty()) {
        return e;
    }
    e.hasBattery = true;

    // State: any charging -> charging; else any discharging -> discharging; else idle.
    const auto any = [&](PowerSupply::Status st) {
        return std::ranges::any_of(batteries,
                                   [st](const PowerSupply* b) { return b->status == st; });
    };
    if (any(PowerSupply::Status::Charging)) {
        e.state = BatteryEstimate::State::Charging;
    } else if (any(PowerSupply::Status::Discharging)) {
        e.state = BatteryEstimate::State::Discharging;
    } else if (any(PowerSupply::Status::Full) || any(PowerSupply::Status::NotCharging)) {
        e.state = BatteryEstimate::State::Idle;
    }

    // Prefer energy (µWh, µW), then charge (µAh, µA): never mix the two families.
    std::optional<double> now = sumAll(batteries, &PowerSupply::energyNow);
    std::optional<double> full = sumAll(batteries, &PowerSupply::energyFull);
    std::optional<double> rate = sumAll(batteries, &PowerSupply::powerNow);
    if (!now || !full) {
        now = sumAll(batteries, &PowerSupply::chargeNow);
        full = sumAll(batteries, &PowerSupply::chargeFull);
        rate = sumAll(batteries, &PowerSupply::currentNow);
    }
    if (now && full && *full > 0.0) {
        e.fraction = std::clamp(*now / *full, 0.0, 1.0);
    } else if (const auto pct = sumAll(batteries, &PowerSupply::capacity)) {
        e.fraction = std::clamp(*pct / kPercent / static_cast<double>(count), 0.0, 1.0);
    }

    if (now && full && rate && std::abs(*rate) > 0.0) {
        const double r = std::abs(*rate); // some firmware reports negative while discharging
        if (e.state == BatteryEstimate::State::Discharging) {
            e.secondsLeft = std::max(*now, 0.0) / r * kSecondsPerHour;
        } else if (e.state == BatteryEstimate::State::Charging) {
            e.secondsLeft = std::max(*full - *now, 0.0) / r * kSecondsPerHour;
        }
    }
    return e;
}

} // namespace qk::sensors
