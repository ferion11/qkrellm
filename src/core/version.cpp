// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/version.h"

#include <qkrellm/core/version.h>

namespace qk::core {

QString versionString()
{
    return QString::fromLatin1(kVersionString);
}

} // namespace qk::core
