// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/netdev.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    static qk::sensors::NetDev previous;
    if (const auto dev = qk::sensors::parseNetDev(text)) {
        if (dev->count > qk::sensors::NetDev::kMaxInterfaces) {
            std::abort();
        }
        for (std::size_t i = 0; i < dev->count; ++i) {
            const auto& cur = dev->interfaces.at(i);
            const auto name = cur.name();
            if (name.empty() || name.size() > qk::sensors::NetInterface::kMaxName) {
                std::abort();
            }
            if (const auto j = previous.find(name)) {
                const auto& old = previous.interfaces.at(*j);
                if (qk::sensors::counterRate(old.rxBytes, cur.rxBytes, 1.0) < 0.0) {
                    std::abort();
                }
            }
        }
        previous = *dev;
    }
    return 0;
}
