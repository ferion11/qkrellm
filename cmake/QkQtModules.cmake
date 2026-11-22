# SPDX-License-Identifier: GPL-3.0-or-later
# ADR 0002 / docs/resource-budget.md: the shipped binary may link only these Qt modules.
# Changing this list requires a new ADR. scripts/check-qt-modules.sh enforces it on the
# built binary (registered as a CTest test).
set(QKRELLM_ALLOWED_QT_MODULES Core Gui Widgets)
