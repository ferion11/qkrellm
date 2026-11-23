# 0011 — Panels render text from bitmap glyph atlases; system fonts are opt-in

- **Status:** Accepted
- **Date:** 2026-11-23
- **Deciders:** maintainer + agent session (M1 text-cost investigation)

## Context

The M0 baseline (ADR 0006) showed a ~120 ms first `drawText` and ~9.8 MiB of private heap
for a window that draws one string. The M1 investigation (scratch experiments, heaptrack,
strace, ltrace — reproducible with `benchmarks/text-cost/`; Qt 6.11.3, offscreen and xcb, machine with 6 240 installed fonts) found:

| Scenario (one 11 px string) | first text ready | private memory |
|---|---|---|
| no text at all | — (first paint ≈ 20 ms) | 1.7 MiB |
| Qt + system fonts (6 240 fonts) | 117 ms offscreen / 130 ms xcb | 11.5 MiB |
| Qt + minimal fontconfig (DejaVu only) | 17 ms | 2.6 MiB |
| pre-rendered glyph strip, `drawPixmap` blits | 3 ms (strip load 1.5 ms) | 1.95 MiB |

- The cost is **Qt populating its font database from fontconfig** (≈ 87 ms) plus font
  matching (≈ 28 ms); glyph rasterisation is ≈ 0.3 ms. Syscalls total ≈ 14 ms: it is CPU.
- heaptrack: peak heap 7.06 MiB, of which ≈ 5.6 MiB is allocated inside `libfontconfig`
  (6 240 allocations — one per installed font).
- Both time and memory therefore scale with **the number of fonts the user has installed**,
  not with anything qkrellm does. Bundling our own font with `addApplicationFont` does not
  help (94 ms): the system database is still populated. Overriding `FONTCONFIG_FILE` would
  break user configuration and dialogs and was rejected.

## Decision

1. All panel text goes through a small `qk::ui::TextRenderer` interface (measure + draw a
   string at a position, colour/role from the theme).
2. **Default implementation: `BitmapTextRenderer`.** It draws glyphs from pre-rendered
   atlases (PNG + per-glyph metrics) compiled into the binary as Qt resources. It must never
   construct a `QFont`/`QFontMetrics` or touch `QFontDatabase`.
   - Atlases are generated **offline** by a developer tool (`tools/glyphgen`, M1) from a
     freely licensed face (DejaVu, Bitstream Vera licence) and **committed** as assets, so
     builds need neither the TTF nor a running Qt GUI, and output does not vary with the
     build host's FreeType.
   - Coverage: ASCII + Latin-1 Supplement (U+0020–U+007E, U+00A0–U+00FF); unsupported
     characters render as `?`. Scales 1× and 2× (HiDPI).
   - Themes (M3) may ship their own atlases in the same format.
3. **Opt-in `SystemFontTextRenderer`** (config option, M3) uses `QPainter::drawText` with a
   user-chosen `QFont`; the font cost is paid only by users who select it, and lazily.
4. Dialogs (preferences, about) keep normal Qt widgets and system fonts; they load fonts
   only when opened, never before the first paint of the monitor window.
5. The startup and memory budgets (ADR 0006) are unchanged and apply to the default
   (bitmap) configuration.

## Consequences

- Startup and private memory become independent of the user's font collection; the
  expected default is ≈ 20 ms to first paint and ≈ 2 MiB private for the empty window.
- No complex scripts, CJK or emoji in panels by default; users needing them enable system
  fonts. Interface names and mount points outside Latin-1 show `?` in bitmap mode.
- New code to own: atlas format, generator tool, renderer, HiDPI selection.
- Guard: a unit test asserts that rendering a panel with the bitmap renderer leaves
  `QFontDatabase` unpopulated (or equivalently that no `libfontconfig` font list was
  built), and `benchmarks/measure.sh` must show the startup budget met on xcb.
