# 0008 — Wayland window placement is an open risk, decided in M5

- **Status:** Proposed
- **Date:** 2026-11-22

## Context

GKrellM-style behaviour (fixed screen position, always-on-top, on all workspaces,
no taskbar entry) is fully available on X11 through window hints. Under Wayland, regular
clients cannot position themselves or force stacking; only compositor-specific protocols
such as wlr-layer-shell (via LayerShellQt, KDE/wlroots compositors) offer it.

## Decision (pending)

Ship X11 behaviour first and keep window-management code isolated in `src/app/`. In M5,
evaluate optional LayerShellQt support (an extra dependency → needs this ADR accepted with
measurements) versus documenting Wayland limitations.

## Consequences

- Until then, Wayland sessions get a normal top-level window.
