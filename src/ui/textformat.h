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

} // namespace qk::ui
