// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/diskstats.h"

#include "sensors/textscan.h"

#include <algorithm>
#include <limits>

namespace qk::sensors {

namespace {

constexpr int kStatFields = 11; // fields after "major minor name" present since 2.6.25
constexpr int kReadsCompleted = 0;
constexpr int kSectorsRead = 2;
constexpr int kWritesCompleted = 4;
constexpr int kSectorsWritten = 6;

bool validName(std::string_view name)
{
    if (name.empty() || name.size() > DiskStat::kMaxName) {
        return false;
    }
    return std::ranges::none_of(
        name, [](char c) { return c == '/' || static_cast<unsigned char>(c) < 0x20 || c == 0x7f; });
}

std::optional<std::uint32_t> parseU32(std::string_view token)
{
    const auto value = parseUnsigned(token);
    if (!value || *value > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(*value);
}

bool parseLine(std::string_view line, DiskStat& out)
{
    const auto major = parseU32(nextToken(line));
    const auto minor = parseU32(nextToken(line));
    const std::string_view name = nextToken(line);
    if (!major || !minor || !validName(name)) {
        return false;
    }
    std::array<std::uint64_t, kStatFields> fields{};
    int count = 0;
    for (std::string_view token = nextToken(line); !token.empty(); token = nextToken(line)) {
        const auto value = parseUnsigned(token);
        if (!value) {
            return false;
        }
        if (count < kStatFields) {
            fields.at(static_cast<std::size_t>(count)) = *value;
        }
        ++count;
    }
    if (count < kStatFields) {
        return false;
    }
    out = DiskStat{};
    std::ranges::copy(name, out.nameStorage.begin());
    out.major = *major;
    out.minor = *minor;
    out.readsCompleted = fields[kReadsCompleted];
    out.sectorsRead = fields[kSectorsRead];
    out.writesCompleted = fields[kWritesCompleted];
    out.sectorsWritten = fields[kSectorsWritten];
    return true;
}

} // namespace

std::optional<std::size_t> DiskStats::find(std::string_view name) const
{
    for (std::size_t i = 0; i < count; ++i) {
        if (devices.at(i).name() == name) {
            return i;
        }
    }
    return std::nullopt;
}

DiskStats parseDiskStats(std::string_view text)
{
    DiskStats stats;
    while (!text.empty()) {
        DiskStat parsed;
        if (!parseLine(nextLine(text), parsed) || stats.find(parsed.name())) {
            continue; // bad line or duplicate: keep going
        }
        if (stats.count == DiskStats::kMaxDevices) {
            stats.truncated = true;
            continue;
        }
        stats.devices.at(stats.count++) = parsed;
    }
    return stats;
}

} // namespace qk::sensors
