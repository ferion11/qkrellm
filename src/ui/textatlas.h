// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace qk::ui {

/// One glyph inside a bitmap atlas (ADR 0011). All values are in atlas (device) pixels.
struct AtlasGlyph
{
    std::uint16_t x = 0; ///< box left inside the atlas image
    std::uint16_t y = 0; ///< box top inside the atlas image
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::int16_t bearingX = 0; ///< pen x -> box left (may be negative)
    std::int16_t bearingY = 0; ///< baseline -> box top, positive upwards
    std::uint16_t advance = 0; ///< pen advance after this glyph
    bool present = false;
};

/// Parsed `.qkatlas` metrics file. Covers code points U+0000–U+00FF (Latin-1).
struct AtlasMetrics
{
    static constexpr char32_t kFallback = U'?';

    int scale = 1;     ///< 1 = normal, 2 = HiDPI atlas
    int pixelSize = 0; ///< nominal font size at scale 1
    int ascent = 0;
    int descent = 0;
    int lineHeight = 0;
    std::array<AtlasGlyph, 256> glyphs{};
};

/// Parses the text format written by tools/glyphgen:
///
///     # comment lines and blank lines are ignored
///     qkatlas 1
///     scale 1
///     pixel_size 11
///     ascent 11
///     descent 3
///     line_height 14
///     g <codepoint> <x> <y> <w> <h> <bearing_x> <bearing_y> <advance>
///
/// Unknown keys are ignored (forward compatibility). Returns std::nullopt for a missing or
/// unsupported header, malformed numbers, out-of-range code points or a missing '?' glyph.
/// Pure function: no I/O, no Qt, no allocation.
[[nodiscard]] std::optional<AtlasMetrics> parseAtlasMetrics(std::string_view text);

} // namespace qk::ui
