// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QStringView>

#include <array>
#include <optional>

namespace qk::ui {

/// Fixed storage for short panel strings; formatting into it never allocates.
using TextBuffer = std::array<char16_t, 32>;

/// "0%".."100%" from a 0..1 fraction (clamped, rounded); "--%" when there is no value.
[[nodiscard]] QStringView formatPercent(TextBuffer& buffer, std::optional<float> fraction);

/// Rounded whole degrees, e.g. "54°C", "-5°C"; "--°C" when there is no value (or NaN).
[[nodiscard]] QStringView formatCelsius(TextBuffer& buffer, std::optional<float> celsius);

/// Rounded whole RPM, e.g. "2400rpm" (negative clamps to 0); "--rpm" when there is no value.
[[nodiscard]] QStringView formatRpm(TextBuffer& buffer, std::optional<float> rpm);

/// Compact binary size: at most 3 significant digits and a one-letter unit (B K M G T P E,
/// powers of 1024), e.g. "512B", "1.5K", "3.2G", "62G", "999M". One decimal below 10 (except
/// bytes); a value of 1000 or more moves to the next unit; capped at "999E". "--" when there is no
/// value (missing, NaN or negative).
[[nodiscard]] QStringView formatBytes(TextBuffer& buffer, std::optional<double> bytes);

/// "<used>/<total>" with formatBytes() for each side, e.g. "5.5G/62G".
[[nodiscard]] QStringView formatBytesOf(TextBuffer& buffer, std::optional<double> used,
                                        std::optional<double> total);

} // namespace qk::ui
