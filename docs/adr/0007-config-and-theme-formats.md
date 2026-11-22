# 0007 — INI config via QSettings; own simple theme format

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

We need persistent settings and themes without extra dependencies (QtCore only for config).
GKrellM compatibility is explicitly out of scope (ADR 0009).

## Decision

- Configuration: `QSettings` in INI format at `$XDG_CONFIG_HOME/qkrellm/qkrellm.conf`.
  Keys are versioned (`[General] configVersion=N`) with explicit migrations.
- Themes: a directory with `theme.ini` (colours, metrics, image references) plus PNG/SVG
  assets. SVG support must not pull QtSvg into the app (pre-rendered PNGs or a decision
  in M3).
- Exact schemas are designed in M3 and documented in `docs/`.

## Consequences

- Config parsing lives in core (QtCore only) and is unit-testable.
- Theme loading lives in ui.
