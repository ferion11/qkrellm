// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

namespace qk::core {

/// Application version as "MAJOR.MINOR.PATCH" (independent from the plugin ABI version).
[[nodiscard]] QString versionString();

} // namespace qk::core
