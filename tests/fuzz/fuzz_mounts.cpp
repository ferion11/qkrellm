// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/mounts.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    const auto mounts = qk::sensors::parseMounts(text);
    const auto disks = qk::sensors::selectDiskMounts(mounts);
    if (disks.size() > mounts.size()) {
        std::abort();
    }
    for (const auto& m : mounts) {
        if (m.fsType.empty() || m.mountPoint.size() > text.size() ||
            m.device.size() > text.size()) {
            std::abort();
        }
    }
    return 0;
}
