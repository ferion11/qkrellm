# 0002 — C++20, CMake presets, Qt ≥ 6.5 limited to Core/Gui/Widgets

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

The project goal is "as light as possible on resources". Every Qt module linked costs
startup time, address space and shared-library RSS. Distribution reach matters too:
Qt 6.5 is the oldest LTS still common in current distros (Debian 13, Fedora, Arch, Gentoo
ship newer).

## Decision

- Language: C++20 (C11 for the plugin ABI and C plugins).
- Build: CMake ≥ 3.25 with `CMakePresets.json` and Ninja; ccache and mold used when found.
- Qt: `find_package(Qt6 6.5 …)`; the shipped binary may link **only Qt6Core, Qt6Gui and
  Qt6Widgets**. The allowlist lives in `cmake/QkQtModules.cmake` and is enforced on the
  built binary by the CTest test `budget.qt_modules` (`scripts/check-qt-modules.sh`).
- Tests may additionally use Qt6Test.

## Consequences

- No QtNetwork, QtDBus, QtSvg, QtQuick… in the app unless a new ADR measures the cost and
  accepts it (e.g. DBus for tray/notifications in M5 would need one).
- Code must avoid Qt APIs newer than 6.5, or guard them with `QT_VERSION_CHECK`; CI builds
  against Qt 6.5 to catch this.
