// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/procstat.h"

#include "sensors/textscan.h"

namespace qk::sensors {

namespace {

constexpr int kMinCpuFields = 4;
constexpr int kMaxCpuFields = 10;

/// Parses the numeric columns of a "cpu" line. Extra (future) columns are ignored.
std::optional<CpuTimes> parseCpuColumns(std::string_view rest)
{
    std::array<std::uint64_t, kMaxCpuFields> values{};
    int count = 0;
    for (std::string_view token = nextToken(rest); !token.empty(); token = nextToken(rest)) {
        const auto value = parseUnsigned(token);
        if (!value) {
            return std::nullopt;
        }
        if (count < kMaxCpuFields) {
            values.at(static_cast<std::size_t>(count)) = *value;
        }
        ++count;
    }
    if (count < kMinCpuFields) {
        return std::nullopt;
    }
    CpuTimes t;
    t.user = values[0];
    t.nice = values[1];
    t.system = values[2];
    t.idle = values[3];
    t.iowait = values[4];
    t.irq = values[5];
    t.softirq = values[6];
    t.steal = values[7];
    t.guest = values[8];
    t.guestNice = values[9];
    t.fields = count < kMaxCpuFields ? count : kMaxCpuFields;
    return t;
}

void parseSingleValue(std::string_view rest, std::optional<std::uint64_t>& out)
{
    out = parseUnsigned(nextToken(rest)); // a malformed value just stays unknown
}

} // namespace

std::optional<ProcStat> parseProcStat(std::string_view text)
{
    ProcStat stat;
    bool haveTotal = false;

    while (!text.empty()) {
        std::string_view rest = nextLine(text);
        const std::string_view key = nextToken(rest);

        if (key == "cpu") {
            const auto times = parseCpuColumns(rest);
            if (!times) {
                return std::nullopt;
            }
            stat.total = *times;
            haveTotal = true;
        } else if (key.size() > 3 && key.starts_with("cpu")) {
            const auto index = parseUnsigned(key.substr(3));
            const auto times = parseCpuColumns(rest);
            if (!index || !times) {
                continue; // one bad per-CPU line must not hide the rest
            }
            if (*index >= static_cast<std::uint64_t>(ProcStat::kMaxCpus)) {
                stat.cpusTruncated = true;
                continue;
            }
            const auto slot = static_cast<int>(*index);
            stat.cpus.at(static_cast<std::size_t>(slot)) = *times;
            stat.cpuSlots = slot + 1 > stat.cpuSlots ? slot + 1 : stat.cpuSlots;
        } else if (key == "ctxt") {
            parseSingleValue(rest, stat.contextSwitches);
        } else if (key == "btime") {
            parseSingleValue(rest, stat.bootTime);
        } else if (key == "processes") {
            parseSingleValue(rest, stat.processesCreated);
        } else if (key == "procs_running") {
            parseSingleValue(rest, stat.procsRunning);
        } else if (key == "procs_blocked") {
            parseSingleValue(rest, stat.procsBlocked);
        } // intr, softirq, page, swap, disk_io... are ignored
    }

    if (!haveTotal) {
        return std::nullopt;
    }
    return stat;
}

} // namespace qk::sensors
