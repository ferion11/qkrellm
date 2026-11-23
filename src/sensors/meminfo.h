// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// The /proc/meminfo fields qkrellm uses, converted to bytes.
/// Kernel docs: Documentation/filesystems/proc.rst, section "meminfo". Lines look like
/// "MemTotal:       65426260 kB"; the set of lines varies with kernel version and config
/// (MemAvailable appeared in 3.14, SReclaimable in 2.6.19), so every field is optional.
/// Fixtures: tests/fixtures/proc/<tag>/proc/meminfo (real captures).
struct MemInfo
{
    std::optional<std::uint64_t> memTotal;
    std::optional<std::uint64_t> memFree;
    std::optional<std::uint64_t> memAvailable; ///< kernel's estimate; absent before 3.14
    std::optional<std::uint64_t> buffers;
    std::optional<std::uint64_t> cached;
    std::optional<std::uint64_t> sReclaimable; ///< reclaimable slab (dentries, inodes)
    std::optional<std::uint64_t> swapTotal;
    std::optional<std::uint64_t> swapFree;
};

/// Parses the text of /proc/meminfo. Unknown, malformed and duplicate-later lines are
/// ignored; values in "kB" are converted to bytes, values with another unit are ignored.
/// Returns std::nullopt when MemTotal is missing, malformed or zero.
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<MemInfo> parseMemInfo(std::string_view text);

/// What the memory panels show, in bytes. Invariants: used + cache <= total,
/// swapUsed <= swapTotal.
struct MemoryUsage
{
    std::uint64_t total = 0;
    std::uint64_t used = 0;  ///< not available to new programs (procps-ng >= 4 definition)
    std::uint64_t cache = 0; ///< buffers + page cache + reclaimable slab, capped to fit
    std::uint64_t swapTotal = 0;
    std::uint64_t swapUsed = 0;
};

/// Derives usage like `free` (procps-ng 4): used = MemTotal - MemAvailable; without
/// MemAvailable (kernel < 3.14), used = MemTotal - MemFree - buff/cache. Saturates instead
/// of wrapping when the kernel's numbers are inconsistent with each other.
[[nodiscard]] MemoryUsage computeMemoryUsage(const MemInfo& info);

} // namespace qk::sensors
