// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/rate.h"

namespace qk::sensors {

double counterRate(std::uint64_t previous, std::uint64_t current, double seconds)
{
    if (current < previous || !(seconds > 0.0)) {
        return 0.0;
    }
    return static_cast<double>(current - previous) / seconds;
}

} // namespace qk::sensors
