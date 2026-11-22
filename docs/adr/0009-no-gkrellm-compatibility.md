# 0009 — No GKrellM theme or gkrellmd protocol compatibility

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

GKrellM has a large theme collection (gkrellmrc + images) and a client/server daemon
(gkrellmd). Supporting either constrains our formats and costs parser/protocol code.

## Decision

qkrellm keeps the *spirit* of GKrellM (compact stacked monitors, krells, charts, themes,
plugins) but uses its own formats. No gkrellmrc theme loader, no gkrellmd client.

## Consequences

- Free to design simpler theme and plugin formats.
- Remote monitoring, if ever wanted, gets its own design.
