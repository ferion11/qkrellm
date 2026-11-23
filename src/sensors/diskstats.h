// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// One line of /proc/diskstats (a disk or a partition).
/// Kernel docs: Documentation/admin-guide/iostats.rst. Each line is
///   "major minor name" + 11 fields (2.6.25+) [+ 4 discard fields (4.18+)]
///                                            [+ 2 flush fields (5.5+)].
/// Fields 3 and 7 count sectors, which are always 512 bytes here regardless of the
/// device's block size. Only the fields used are kept.
/// Fixtures: tests/fixtures/proc/<tag>/proc/diskstats{,.1s} (real captures).
struct DiskStat
{
    static constexpr std::size_t kMaxName = 31; ///< DISK_NAME_LEN - 1
    static constexpr std::uint64_t kSectorBytes = 512;

    std::array<char, kMaxName + 1> nameStorage{}; ///< NUL-terminated
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint64_t readsCompleted = 0;
    std::uint64_t sectorsRead = 0;
    std::uint64_t writesCompleted = 0;
    std::uint64_t sectorsWritten = 0;

    [[nodiscard]] std::string_view name() const { return {nameStorage.data()}; }
};

/// Parsed /proc/diskstats. Fixed-size so a reader can hold one without heap allocation.
struct DiskStats
{
    /// Lines beyond this count are ignored and `truncated` is set (partitions, loop and
    /// ram devices all count).
    static constexpr std::size_t kMaxDevices = 256;

    std::array<DiskStat, kMaxDevices> devices{};
    std::size_t count = 0;
    bool truncated = false;

    /// Index of the device called `name`, if present.
    [[nodiscard]] std::optional<std::size_t> find(std::string_view name) const;
};

/// Counters of one block device from /sys/block/<name>/stat: the same statistics fields as a
/// /proc/diskstats line, without "major minor name" (Documentation/block/stat.rst). Reading
/// it is several times cheaper than /proc/diskstats, which lists every ram/loop/partition.
struct BlockStat
{
    std::uint64_t readsCompleted = 0;
    std::uint64_t sectorsRead = 0;
    std::uint64_t writesCompleted = 0;
    std::uint64_t sectorsWritten = 0;
};

/// Parses /sys/block/<name>/stat: at least 11 numeric fields (extra ones ignored);
/// std::nullopt otherwise. Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<BlockStat> parseBlockStat(std::string_view text);

/// Parses the text of /proc/diskstats. A line with fewer than the 11 statistics fields,
/// non-numeric values, or an invalid name (empty, longer than 31, containing '/' or
/// control characters) is ignored; duplicates keep the first. Never fails: an empty or
/// unrelated text yields zero devices.
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] DiskStats parseDiskStats(std::string_view text);

} // namespace qk::sensors
