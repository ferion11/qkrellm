// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/textatlas.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    (void)qk::ui::parseAtlasMetrics(std::string_view(reinterpret_cast<const char*>(data), size));
    return 0;
}
