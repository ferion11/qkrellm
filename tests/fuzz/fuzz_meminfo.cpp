// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/meminfo.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    if (const auto info = qk::sensors::parseMemInfo(text)) {
        const auto usage = qk::sensors::computeMemoryUsage(*info);
        // Documented invariants (computeMemoryUsage): checked under the fuzzer.
        if (usage.used > usage.total || usage.cache > usage.total - usage.used ||
            usage.swapUsed > usage.swapTotal) {
            std::abort();
        }
    }
    return 0;
}
