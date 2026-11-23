// SPDX-License-Identifier: GPL-3.0-or-later
// Small allocation-free helpers shared by the kernel text parsers.
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// Removes and returns the next line (without '\n') from `text`.
[[nodiscard]] std::string_view nextLine(std::string_view& text);

/// Removes and returns the next token separated by spaces/tabs from `text`.
[[nodiscard]] std::string_view nextToken(std::string_view& text);

/// Decimal digits only; std::nullopt on empty input, non-digits or uint64 overflow.
[[nodiscard]] std::optional<std::uint64_t> parseUnsigned(std::string_view digits);

} // namespace qk::sensors
