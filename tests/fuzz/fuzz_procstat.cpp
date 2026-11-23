// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/cpuusage.h"
#include "sensors/procstat.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): fuzzer bytes as text
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    static qk::sensors::ProcStat previous; // ~20 KiB: static, not on the stack
    if (const auto stat = qk::sensors::parseProcStat(text)) {
        (void)qk::sensors::computeUsage(previous.total, stat->total);
        for (int i = 0; i < stat->cpuSlots; ++i) {
            const auto idx = static_cast<std::size_t>(i);
            (void)qk::sensors::computeUsage(previous.cpus.at(idx), stat->cpus.at(idx));
        }
        previous = *stat;
    }
    return 0;
}
