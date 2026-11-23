// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/loadavg.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    const auto load = qk::sensors::parseLoadAvg(text);
    if (load && !(load->one >= 0.0 && load->five >= 0.0 && load->fifteen >= 0.0)) {
        std::abort();
    }
    return 0;
}
