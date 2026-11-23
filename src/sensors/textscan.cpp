// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/textscan.h"

#include <limits>

namespace qk::sensors {

std::string_view nextLine(std::string_view& text)
{
    const std::size_t eol = text.find('\n');
    const std::string_view line = text.substr(0, eol);
    text.remove_prefix(eol == std::string_view::npos ? text.size() : eol + 1);
    return line;
}

std::string_view nextToken(std::string_view& text)
{
    const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    std::size_t end = 0;
    while (end < text.size() && !isSpace(text[end])) {
        ++end;
    }
    const std::string_view token = text.substr(0, end);
    text.remove_prefix(end);
    return token;
}

std::optional<std::uint64_t> parseUnsigned(std::string_view digits)
{
    if (digits.empty()) {
        return std::nullopt;
    }
    constexpr std::uint64_t kMax = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t value = 0;
    for (const char c : digits) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        const auto digit = static_cast<std::uint64_t>(c - '0');
        if (value > (kMax - digit) / 10) {
            return std::nullopt;
        }
        value = (value * 10) + digit;
    }
    return value;
}

} // namespace qk::sensors
