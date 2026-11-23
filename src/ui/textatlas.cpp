// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/textatlas.h"

#include <limits>

namespace qk::ui {

namespace {

/// Splits off the next whitespace-separated token from `text`.
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

std::optional<long> parseInteger(std::string_view token)
{
    bool negative = false;
    if (!token.empty() && token.front() == '-') {
        negative = true;
        token.remove_prefix(1);
    }
    if (token.empty() || token.size() > 6) {
        return std::nullopt; // atlases are small: anything longer is garbage
    }
    long value = 0;
    for (const char c : token) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        value = (value * 10) + (c - '0');
    }
    return negative ? -value : value;
}

template<typename T>
bool parseInto(std::string_view& rest, T& out)
{
    const auto value = parseInteger(nextToken(rest));
    if (!value || *value < std::numeric_limits<T>::min() ||
        *value > std::numeric_limits<T>::max()) {
        return false;
    }
    out = static_cast<T>(*value);
    return true;
}

bool parseGlyph(std::string_view rest, AtlasMetrics& metrics)
{
    std::uint8_t codepoint = 0;
    AtlasGlyph glyph;
    if (!parseInto(rest, codepoint) || !parseInto(rest, glyph.x) || !parseInto(rest, glyph.y) ||
        !parseInto(rest, glyph.width) || !parseInto(rest, glyph.height) ||
        !parseInto(rest, glyph.bearingX) || !parseInto(rest, glyph.bearingY) ||
        !parseInto(rest, glyph.advance)) {
        return false;
    }
    glyph.present = true;
    metrics.glyphs[codepoint] = glyph;
    return true;
}

} // namespace

std::optional<AtlasMetrics> parseAtlasMetrics(std::string_view text)
{
    AtlasMetrics metrics;
    bool sawHeader = false;

    while (!text.empty()) {
        const std::size_t eol = text.find('\n');
        const std::string_view line = text.substr(0, eol);
        text.remove_prefix(eol == std::string_view::npos ? text.size() : eol + 1);

        std::string_view rest = line;
        const std::string_view key = nextToken(rest);
        if (key.empty() || key.front() == '#') {
            continue;
        }
        if (!sawHeader) {
            int version = 0;
            if (key != "qkatlas" || !parseInto(rest, version) || version != 1) {
                return std::nullopt;
            }
            sawHeader = true;
            continue;
        }

        bool ok = true;
        if (key == "g") {
            ok = parseGlyph(rest, metrics);
        } else if (key == "scale") {
            ok = parseInto(rest, metrics.scale);
        } else if (key == "pixel_size") {
            ok = parseInto(rest, metrics.pixelSize);
        } else if (key == "ascent") {
            ok = parseInto(rest, metrics.ascent);
        } else if (key == "descent") {
            ok = parseInto(rest, metrics.descent);
        } else if (key == "line_height") {
            ok = parseInto(rest, metrics.lineHeight);
        } // unknown keys are ignored on purpose
        if (!ok) {
            return std::nullopt;
        }
    }

    const bool sane = sawHeader && (metrics.scale == 1 || metrics.scale == 2) &&
                      metrics.pixelSize > 0 && metrics.lineHeight > 0 &&
                      metrics.glyphs[AtlasMetrics::kFallback].present;
    if (!sane) {
        return std::nullopt;
    }
    return metrics;
}

} // namespace qk::ui
