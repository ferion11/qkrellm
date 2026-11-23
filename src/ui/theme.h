// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>

namespace qk::ui {

/// Visual parameters shared by panels. M1 uses the built-in dark defaults below; the theme
/// engine (M3, ADR 0007) will load these from theme.ini.
struct Theme
{
    QColor windowBackground{0x20, 0x23, 0x26};
    QColor chartBackground{0x10, 0x12, 0x14};
    QColor chartGrid{0x2a, 0x2e, 0x33};
    QColor text{0xd8, 0xde, 0xe9};
    QColor dimText{0x8a, 0x93, 0xa0};
    QColor seriesPrimary{0x4f, 0xb3, 0xff};   ///< e.g. CPU user
    QColor seriesSecondary{0xff, 0x8a, 0x3d}; ///< e.g. CPU system, swap used
    QColor seriesMuted{0x3a, 0x5a, 0x78};     ///< e.g. memory cache (reclaimable)
    int panelWidth = 120;                     ///< logical px (history may be longer)
    int chartHeight = 36;                     ///< logical px
    int panelPadding = 2;                     ///< logical px around header text
};

} // namespace qk::ui
