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

QStringView formatCelsius(TextBuffer& buffer, std::optional<float> celsius)
{
    constexpr float kLimit = 9999.0F; // keeps the text short whatever the sensor reports
    std::size_t n = 0;
    if (!celsius || std::isnan(*celsius)) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
    } else {
        const float clamped = std::clamp(*celsius, -kLimit, kLimit);
        const long long whole = std::llround(clamped);
        if (whole < 0) {
            buffer.at(n++) = u'-';
        }
        appendUnsigned(buffer, n, whole < 0 ? -whole : whole);
    }
    buffer.at(n++) = u'\u00B0';
    buffer.at(n++) = u'C';
    return view(buffer, n);
}

QStringView formatRpm(TextBuffer& buffer, std::optional<float> rpm)
{
    constexpr float kLimit = 999999.0F;
    std::size_t n = 0;
    if (!rpm || std::isnan(*rpm)) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
    } else {
        appendUnsigned(buffer, n, std::llround(std::clamp(*rpm, 0.0F, kLimit)));
    }
    for (const char16_t c : {u'r', u'p', u'm'}) {
        buffer.at(n++) = c;
    }
    return view(buffer, n);
}

namespace {

void appendDuration(TextBuffer& buffer, std::size_t& n, std::optional<double> seconds)
{
    if (!seconds || std::isnan(*seconds) || *seconds < 0.0) {
        for (const char16_t c : {u'-', u'-', u':', u'-', u'-'}) {
            buffer.at(n++) = c;
        }
        return;
    }
    constexpr long long kMaxMinutes = (99 * 60) + 59;
    const long long minutes = std::min(std::llround(std::min(*seconds, 1e9) / 60.0), kMaxMinutes);
    appendUnsigned(buffer, n, minutes / 60);
    buffer.at(n++) = u':';
    buffer.at(n++) = static_cast<char16_t>(u'0' + ((minutes % 60) / 10));
    buffer.at(n++) = static_cast<char16_t>(u'0' + (minutes % 10));
}

} // namespace

QStringView formatDuration(TextBuffer& buffer, std::optional<double> seconds)
{
    std::size_t n = 0;
    appendDuration(buffer, n, seconds);
    return view(buffer, n);
}

QStringView formatUptime(TextBuffer& buffer, std::optional<double> seconds)
{
    std::size_t n = 0;
    if (!seconds || std::isnan(*seconds) || *seconds < 0.0) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
        return view(buffer, n);
    }
    constexpr long long kMinutesPerDay = 24LL * 60;
    const auto minutes = static_cast<long long>(std::min(*seconds, 1e11) / 60.0);
    const long long days = minutes / kMinutesPerDay;
    if (days > 0) {
        appendUnsigned(buffer, n, days);
        buffer.at(n++) = u'd';
        buffer.at(n++) = u' ';
    }
    const long long rest = minutes % kMinutesPerDay;
    appendUnsigned(buffer, n, rest / 60);
    buffer.at(n++) = u':';
    buffer.at(n++) = static_cast<char16_t>(u'0' + ((rest % 60) / 10));
    buffer.at(n++) = static_cast<char16_t>(u'0' + (rest % 10));
    return view(buffer, n);
}

QStringView formatLoad(TextBuffer& buffer, std::optional<float> load)
{
    std::size_t n = 0;
    if (!load || std::isnan(*load) || *load < 0.0F) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
        return view(buffer, n);
    }
    const double value = std::min(static_cast<double>(*load), 99999.0);
    const long long hundredths = std::llround(value * 100.0);
    const long long tenths = std::llround(value * 10.0);
    if (hundredths < 1000) { // "9.99"; 9.996 rounds to 1000 and prints "10.0"
        appendUnsigned(buffer, n, hundredths / 100);
        buffer.at(n++) = u'.';
        appendUnsigned(buffer, n, (hundredths % 100) / 10);
        appendUnsigned(buffer, n, hundredths % 10);
    } else if (tenths < 1000) {
        appendUnsigned(buffer, n, tenths / 10);
        buffer.at(n++) = u'.';
        appendUnsigned(buffer, n, tenths % 10);
    } else {
        appendUnsigned(buffer, n, std::llround(value));
    }
    return view(buffer, n);
}

QStringView formatPair(TextBuffer& buffer, int first, char16_t separator, int second)
{
    std::size_t n = 0;
    const auto twoDigits = [&](int value) {
        const int v = std::clamp(value, 0, 99);
        buffer.at(n++) = static_cast<char16_t>(u'0' + (v / 10));
        buffer.at(n++) = static_cast<char16_t>(u'0' + (v % 10));
    };
    twoDigits(first);
    buffer.at(n++) = separator;
    twoDigits(second);
    return view(buffer, n);
}

QStringView formatBattery(TextBuffer& buffer, std::optional<float> fraction,
                          std::optional<double> secondsLeft)
{
    const QStringView percent = formatPercent(buffer, fraction);
    auto n = static_cast<std::size_t>(percent.size());
    if (secondsLeft && !std::isnan(*secondsLeft) && *secondsLeft >= 0.0) {
        buffer.at(n++) = u' ';
        appendDuration(buffer, n, secondsLeft);
    }
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
