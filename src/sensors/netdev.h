// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// Counters of one interface in /proc/net/dev.
/// Kernel: net/core/net-procfs.c (dev_seq_printf_stats). Each data line is
///   "<name>: rx_bytes rx_packets errs drop fifo frame compressed multicast
///            tx_bytes tx_packets errs drop fifo colls carrier compressed"
/// The name is right-aligned in a 6-column field and, on old kernels, large rx_bytes
/// values follow the colon without a space ("eth0:123456789"). The 16 numeric columns
/// have been stable since 2.6; only the four used here are kept.
/// Fixtures: tests/fixtures/proc/<tag>/proc/net/dev{,.1s} (real captures).
struct NetInterface
{
    static constexpr std::size_t kMaxName = 15; ///< IFNAMSIZ - 1

    std::array<char, kMaxName + 1> nameStorage{}; ///< NUL-terminated
    std::uint64_t rxBytes = 0;
    std::uint64_t rxPackets = 0;
    std::uint64_t txBytes = 0;
    std::uint64_t txPackets = 0;

    [[nodiscard]] std::string_view name() const { return {nameStorage.data()}; }
};

/// Parsed /proc/net/dev. Fixed-size so a reader can hold one without heap allocation.
struct NetDev
{
    /// Interfaces beyond this count are ignored and `truncated` is set.
    static constexpr std::size_t kMaxInterfaces = 64;

    std::array<NetInterface, kMaxInterfaces> interfaces{};
    std::size_t count = 0;
    bool truncated = false;

    /// Index of the interface called `name`, if present.
    [[nodiscard]] std::optional<std::size_t> find(std::string_view name) const;
};

/// Parses the text of /proc/net/dev. The two header lines are skipped; a line without
/// "name:" followed by at least the 10 numeric columns up to tx_packets, or with a name
/// longer than 15 characters, is ignored. Returns std::nullopt only when the text does not
/// start with the expected header (not a /proc/net/dev file).
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<NetDev> parseNetDev(std::string_view text);

} // namespace qk::sensors
