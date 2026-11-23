// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/loadavg.h"

#include "sensors/textscan.h"

namespace qk::sensors {

namespace {

/// "12" or "12.34". Locale-independent on purpose (strtod is not).
std::optional<double> parseDecimal(std::string_view token)
{
    constexpr std::size_t kMaxFractionDigits = 9;
    const auto dot = token.find('.');
    const auto whole = parseUnsigned(token.substr(0, dot));
    if (!whole) {
        return std::nullopt;
    }
    auto value = static_cast<double>(*whole);
    if (dot == std::string_view::npos) {
        return value;
    }
    const std::string_view digits = token.substr(dot + 1);
    const auto fraction = parseUnsigned(digits);
    if (!fraction || digits.size() > kMaxFractionDigits) {
        return std::nullopt;
    }
    double divisor = 1.0;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        divisor *= 10.0;
    }
    value += static_cast<double>(*fraction) / divisor;
    return value;
}

} // namespace

std::optional<LoadAvg> parseLoadAvg(std::string_view text)
{
    std::string_view line = nextLine(text);
    const auto one = parseDecimal(nextToken(line));
    const auto five = parseDecimal(nextToken(line));
    const auto fifteen = parseDecimal(nextToken(line));
    if (!one || !five || !fifteen) {
        return std::nullopt;
    }
    return LoadAvg{.one = *one, .five = *five, .fifteen = *fifteen};
}

} // namespace qk::sensors
