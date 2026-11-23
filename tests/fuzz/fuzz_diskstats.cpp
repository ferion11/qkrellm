// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/diskstats.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    static qk::sensors::DiskStats stats; // ~14 KiB: static, not on the stack
    stats = qk::sensors::parseDiskStats(text);
    (void)qk::sensors::parseBlockStat(text);
    if (stats.count > qk::sensors::DiskStats::kMaxDevices) {
        std::abort();
    }
    for (std::size_t i = 0; i < stats.count; ++i) {
        const auto name = stats.devices.at(i).name();
        if (name.empty() || name.size() > qk::sensors::DiskStat::kMaxName) {
            std::abort();
        }
    }
    return 0;
}
