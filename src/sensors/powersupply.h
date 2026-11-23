// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace qk::sensors {

/// The fields qkrellm uses from /sys/class/power_supply/<name>/uevent.
/// Kernel docs: Documentation/ABI/testing/sysfs-class-power. The file is "KEY=VALUE" lines
/// with keys "POWER_SUPPLY_<PROPERTY>". Energy is in µWh / µW, charge in µAh / µA; a
/// battery reports one family or the other (or only CAPACITY).
/// Fixtures: tests/fixtures/sys/<tag>/class/power_supply/<name>/uevent (real captures).
struct PowerSupply
{
    enum class Type : std::uint8_t { Unknown, Battery, Mains, Usb, Other };
    enum class Status : std::uint8_t { Unknown, Charging, Discharging, NotCharging, Full };

    Type type = Type::Unknown;
    Status status = Status::Unknown;
    bool deviceScope = false; ///< SCOPE=Device: a peripheral's battery (mouse, pen), not ours
    std::optional<bool> present;
    std::optional<bool> online;           ///< mains/USB adapters
    std::optional<std::int64_t> capacity; ///< percent, 0..100 as reported
    std::optional<std::int64_t> energyNow;
    std::optional<std::int64_t> energyFull;
    std::optional<std::int64_t> powerNow;
    std::optional<std::int64_t> chargeNow;
    std::optional<std::int64_t> chargeFull;
    std::optional<std::int64_t> currentNow;
};

/// Parses a power_supply uevent. Unknown keys and malformed values are ignored; never
/// fails (an unrelated text yields Type::Unknown).
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] PowerSupply parsePowerSupplyUevent(std::string_view text);

/// What the battery panel shows, combining every system battery (laptops may have two).
struct BatteryEstimate
{
    enum class State : std::uint8_t {
        Unknown,
        Charging,
        Discharging,
        Idle, ///< full or not charging
    };

    bool hasBattery = false;
    std::optional<double> fraction; ///< 0..1 of full charge
    State state = State::Unknown;
    std::optional<double> secondsLeft; ///< to empty (discharging) or to full (charging)
    bool acOnline = false;             ///< any mains/USB supply online
};

/// Combines supplies: batteries with SCOPE=Device or PRESENT=0 are ignored. Fraction from
/// summed energy (or charge) now/full, else the mean CAPACITY. Time left from the summed
/// rate (power or current); unknown when the rate is zero or the families are mixed.
/// Pure function.
[[nodiscard]] BatteryEstimate estimateBattery(std::span<const PowerSupply> supplies);

} // namespace qk::sensors
