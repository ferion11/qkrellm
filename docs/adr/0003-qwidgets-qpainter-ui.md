# 0003 — UI with QWidgets + QPainter, no QML

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

A GKrellM-style monitor is a narrow stack of small charts, meters ("krells") and text
decals that update ~1×/s. QML/Qt Quick brings a scene graph, a JS engine and usually GPU
context setup: tens of MB and noticeable startup for no visual benefit here.

## Decision

All UI is QWidgets with custom `paintEvent` using `QPainter` (raster). One top-level
window; panels are child widgets. No QML, no Qt Quick, no OpenGL widgets.

## Consequences

- Repaints must be targeted (`update(rect)` on widgets whose data changed), opaque widgets
  set `Qt::WA_OpaquePaintEvent`, and pixmaps for static theme parts are cached.
- Measured in M0: the first `QPainter::drawText` costs ~120 ms (fontconfig + glyph
  rasterisation) while a text-free first paint takes ~20 ms. Text rendering strategy is an
  open question for M1/M3 (see `docs/resource-budget.md`).
