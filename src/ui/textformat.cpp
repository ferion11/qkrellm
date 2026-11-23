// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/textformat.h"

#include <algorithm>
#include <cmath>

namespace qk::ui {

QStringView formatPercent(TextBuffer& buffer, std::optional<float> fraction)
{
    std::size_t n = 0;
    if (!fraction || std::isnan(*fraction)) {
        buffer.at(n++) = u'-';
        buffer.at(n++) = u'-';
    } else {
        const int percent = std::clamp(static_cast<int>(std::lround(*fraction * 100.0F)), 0, 100);
        std::array<char16_t, 3> digits{};
        std::size_t count = 0;
        for (int value = percent; count == 0 || value > 0; value /= 10) {
            digits.at(count++) = static_cast<char16_t>(u'0' + (value % 10));
        }
        while (count > 0) {
            buffer.at(n++) = digits.at(--count);
        }
    }
    buffer.at(n++) = u'%';
    return {buffer.data(), static_cast<qsizetype>(n)};
}

} // namespace qk::ui
