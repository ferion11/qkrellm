// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/textformat.h"

#include <algorithm>
#include <cmath>

namespace qk::ui {

namespace {

/// Appends the decimal digits of `value` (>= 0) at buffer[n].
void appendUnsigned(TextBuffer& buffer, std::size_t& n, long long value)
{
    std::array<char16_t, 20> digits{};
    std::size_t count = 0;
    for (; count == 0 || value > 0; value /= 10) {
        digits.at(count++) = static_cast<char16_t>(u'0' + (value % 10));
    }
    while (count > 0) {
        buffer.at(n++) = digits.at(--count);
    }
}

void appendBytes(TextBuffer& buffer, std::size_t& n, std::optional<double> bytes)
{
    if (!bytes || std::isnan(*bytes) || *bytes < 0.0) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
        return;
    }
    static constexpr std::array<char16_t, 7> kUnits{u'B', u'K', u'M', u'G', u'T', u'P', u'E'};
    constexpr double kStep = 1024.0;
    constexpr double kNextUnitAt = 1000.0; // keeps at most 3 digits before the unit
    double value = *bytes;
    std::size_t unit = 0;
    while (value >= kNextUnitAt && unit + 1 < kUnits.size()) {
        value /= kStep;
        ++unit;
    }
    value = std::min(value, kNextUnitAt - 1.0); // beyond 999E (impossible for uint64): cap
    const long long tenths = std::llround(value * 10.0);
    if (unit > 0 && tenths < 100) { // "3.2G"; 9.96 rounds to 100 tenths and prints "10G"
        appendUnsigned(buffer, n, tenths / 10);
        buffer.at(n++) = u'.';
        appendUnsigned(buffer, n, tenths % 10);
    } else {
        appendUnsigned(buffer, n, std::llround(value));
    }
    buffer.at(n++) = kUnits.at(unit);
}

QStringView view(const TextBuffer& buffer, std::size_t n)
{
    return {buffer.data(), static_cast<qsizetype>(n)};
}

} // namespace

QStringView formatPercent(TextBuffer& buffer, std::optional<float> fraction)
{
    std::size_t n = 0;
    if (!fraction || std::isnan(*fraction)) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
    } else {
        appendUnsigned(
            buffer, n,
            std::clamp(static_cast<long long>(std::lround(*fraction * 100.0F)), 0LL, 100LL));
    }
    buffer.at(n++) = u'%';
    return view(buffer, n);
}

QStringView formatBytes(TextBuffer& buffer, std::optional<double> bytes)
{
    std::size_t n = 0;
    appendBytes(buffer, n, bytes);
    return view(buffer, n);
}

QStringView formatBytesOf(TextBuffer& buffer, std::optional<double> used,
                          std::optional<double> total)
{
    std::size_t n = 0;
    appendBytes(buffer, n, used);
    buffer.at(n++) = u'/';
    appendBytes(buffer, n, total);
    return view(buffer, n);
}

} // namespace qk::ui
