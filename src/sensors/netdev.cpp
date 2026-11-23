// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/netdev.h"

#include "sensors/textscan.h"

#include <algorithm>

namespace qk::sensors {

namespace {

constexpr int kRxBytes = 0;
constexpr int kRxPackets = 1;
constexpr int kTxBytes = 8;
constexpr int kTxPackets = 9;
constexpr int kRequiredColumns = kTxPackets + 1;

std::string_view trimSpaces(std::string_view s)
{
    const auto first = s.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

/// Parses one data line into `out`. False when the line is not a valid interface line.
bool parseInterfaceLine(std::string_view line, NetInterface& out)
{
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) {
        return false;
    }
    const std::string_view name = trimSpaces(line.substr(0, colon));
    // Kernel interface names never contain whitespace, '/' or ':' (dev_valid_name()); a
    // NUL would also truncate the stored C string, so it is rejected too.
    constexpr std::string_view kInvalid{" \t\n\v\f\r/\0", 8};
    if (name.empty() || name.size() > NetInterface::kMaxName ||
        name.find_first_of(kInvalid) != std::string_view::npos) {
        return false;
    }
    std::string_view rest = line.substr(colon + 1);
    std::array<std::uint64_t, kRequiredColumns> columns{};
    int count = 0;
    for (std::string_view token = nextToken(rest); !token.empty(); token = nextToken(rest)) {
        const auto value = parseUnsigned(token);
        if (!value) {
            return false;
        }
        if (count < kRequiredColumns) {
            columns.at(static_cast<std::size_t>(count)) = *value;
        }
        ++count;
    }
    if (count < kRequiredColumns) {
        return false;
    }
    out = NetInterface{};
    std::ranges::copy(name, out.nameStorage.begin());
    out.rxBytes = columns[kRxBytes];
    out.rxPackets = columns[kRxPackets];
    out.txBytes = columns[kTxBytes];
    out.txPackets = columns[kTxPackets];
    return true;
}

} // namespace

std::optional<std::size_t> NetDev::find(std::string_view name) const
{
    for (std::size_t i = 0; i < count; ++i) {
        if (interfaces.at(i).name() == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<NetDev> parseNetDev(std::string_view text)
{
    // Header: "Inter-|   Receive ..." and " face |bytes    packets ...".
    if (!text.starts_with("Inter-|")) {
        return std::nullopt;
    }
    (void)nextLine(text);
    (void)nextLine(text);

    NetDev dev;
    while (!text.empty()) {
        const std::string_view line = nextLine(text);
        NetInterface parsed;
        if (!parseInterfaceLine(line, parsed)) {
            continue; // one bad line must not hide the other interfaces
        }
        if (dev.find(parsed.name())) {
            continue; // duplicate name: keep the first
        }
        if (dev.count == NetDev::kMaxInterfaces) {
            dev.truncated = true;
            continue;
        }
        dev.interfaces.at(dev.count++) = parsed;
    }
    return dev;
}

double counterRate(std::uint64_t previous, std::uint64_t current, double seconds)
{
    if (current < previous || !(seconds > 0.0)) {
        return 0.0;
    }
    return static_cast<double>(current - previous) / seconds;
}

} // namespace qk::sensors
