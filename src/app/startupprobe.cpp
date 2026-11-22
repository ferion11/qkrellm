// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/startupprobe.h"

#include <charconv>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <string_view>

#include <unistd.h>

namespace qk::app {

namespace {

// Field 22 of /proc/<pid>/stat; the comm field (2) may contain spaces, so count from
// the last ')'. After it, field 3 is index 0, hence field 22 is index 19.
constexpr int kStartTimeIndexAfterComm = 19;

std::optional<unsigned long long> parseUnsigned(std::string_view digits)
{
    if (digits.empty()) {
        return std::nullopt;
    }
    unsigned long long value = 0;
    for (const char c : digits) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        value = (value * 10) + static_cast<unsigned long long>(c - '0');
    }
    return value;
}

std::optional<unsigned long long> readStartTimeTicks()
{
    std::FILE* file = std::fopen("/proc/self/stat", "re");
    if (file == nullptr) {
        return std::nullopt;
    }
    char buffer[1024];
    const std::size_t len = std::fread(buffer, 1, sizeof(buffer) - 1, file);
    std::fclose(file);

    std::string_view text(buffer, len);
    const auto commEnd = text.rfind(')');
    if (commEnd == std::string_view::npos) {
        return std::nullopt;
    }
    text.remove_prefix(commEnd + 1);

    int index = -1;
    while (!text.empty()) {
        while (!text.empty() && text.front() == ' ') {
            text.remove_prefix(1);
        }
        const auto end = text.find(' ');
        const std::string_view field = text.substr(0, end);
        if (++index == kStartTimeIndexAfterComm) {
            return parseUnsigned(field);
        }
        if (end == std::string_view::npos) {
            break;
        }
        text.remove_prefix(end);
    }
    return std::nullopt;
}

} // namespace

std::optional<long long> msSinceProcessStart()
{
    const auto startTicks = readStartTimeTicks();
    const long ticksPerSecond = sysconf(_SC_CLK_TCK);
    timespec now{};
    if (!startTicks || ticksPerSecond <= 0 || clock_gettime(CLOCK_BOOTTIME, &now) != 0) {
        return std::nullopt;
    }
    const long long nowMs = (static_cast<long long>(now.tv_sec) * 1000) + (now.tv_nsec / 1'000'000);
    const long long startMs = static_cast<long long>(*startTicks) * 1000 / ticksPerSecond;
    return nowMs - startMs;
}

} // namespace qk::app
