// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/powersupply.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    // Split the input in two supplies so the combination logic is fuzzed as well.
    const std::size_t half = text.size() / 2;
    const std::array supplies{qk::sensors::parsePowerSupplyUevent(text.substr(0, half)),
                              qk::sensors::parsePowerSupplyUevent(text.substr(half))};
    const auto e = qk::sensors::estimateBattery(supplies);
    if (e.fraction && !(*e.fraction >= 0.0 && *e.fraction <= 1.0)) {
        std::abort();
    }
    if (e.secondsLeft && !(*e.secondsLeft >= 0.0)) {
        std::abort();
    }
    return 0;
}
