// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/meminfo.h"

#include "sensors/textscan.h"

#include <algorithm>
#include <limits>

namespace qk::sensors {

namespace {

constexpr std::uint64_t kKiB = 1024;

/// "  65426260 kB" -> bytes. std::nullopt for anything else (other unit, garbage, overflow).
std::optional<std::uint64_t> parseKiloBytes(std::string_view rest)
{
    const auto value = parseUnsigned(nextToken(rest));
    if (!value || nextToken(rest) != "kB" || !nextToken(rest).empty()) {
        return std::nullopt;
    }
    if (*value > std::numeric_limits<std::uint64_t>::max() / kKiB) {
        return std::nullopt;
    }
    return *value * kKiB;
}

std::uint64_t saturatingSub(std::uint64_t a, std::uint64_t b)
{
    return a > b ? a - b : 0;
}

std::uint64_t saturatingAdd(std::uint64_t a, std::uint64_t b)
{
    return b > std::numeric_limits<std::uint64_t>::max() - a
               ? std::numeric_limits<std::uint64_t>::max()
               : a + b;
}

} // namespace

std::optional<MemInfo> parseMemInfo(std::string_view text)
{
    MemInfo info;
    while (!text.empty()) {
        std::string_view rest = nextLine(text);
        const std::string_view key = nextToken(rest);
        std::optional<std::uint64_t>* field = nullptr;
        if (key == "MemTotal:") {
            field = &info.memTotal;
        } else if (key == "MemFree:") {
            field = &info.memFree;
        } else if (key == "MemAvailable:") {
            field = &info.memAvailable;
        } else if (key == "Buffers:") {
            field = &info.buffers;
        } else if (key == "Cached:") {
            field = &info.cached;
        } else if (key == "SReclaimable:") {
            field = &info.sReclaimable;
        } else if (key == "SwapTotal:") {
            field = &info.swapTotal;
        } else if (key == "SwapFree:") {
            field = &info.swapFree;
        }
        if (field != nullptr && !field->has_value()) { // first occurrence wins
            *field = parseKiloBytes(rest);
        }
    }
    if (!info.memTotal || *info.memTotal == 0) {
        return std::nullopt;
    }
    return info;
}

MemoryUsage computeMemoryUsage(const MemInfo& info)
{
    MemoryUsage usage;
    usage.total = info.memTotal.value_or(0);
    const std::uint64_t buffCache =
        saturatingAdd(saturatingAdd(info.buffers.value_or(0), info.cached.value_or(0)),
                      info.sReclaimable.value_or(0));
    if (info.memAvailable) {
        usage.used = saturatingSub(usage.total, *info.memAvailable);
    } else {
        usage.used = saturatingSub(saturatingSub(usage.total, info.memFree.value_or(0)), buffCache);
    }
    usage.cache = std::min(buffCache, usage.total - usage.used);

    usage.swapTotal = info.swapTotal.value_or(0);
    usage.swapUsed = saturatingSub(usage.swapTotal, info.swapFree.value_or(0));
    return usage;
}

} // namespace qk::sensors
