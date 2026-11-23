# Bitmap glyph atlases

Panel text is drawn from these pre-rendered atlases instead of system fonts (ADR 0011).

| File | What |
|---|---|
| `default.png`, `default.qkatlas` | DejaVu Sans 11 px, scale 1 |
| `default@2x.png`, `default@2x.qkatlas` | same at scale 2 (HiDPI) |
| `LICENSE.DejaVu` | licence of the DejaVu fonts the atlases are rendered from |

- Coverage: U+0020–U+007E and U+00A0–U+00FF; anything else renders as `?`.
- Format: see `src/ui/textatlas.h`. Generated files — **do not edit by hand**.
- Regenerate with `tools/glyphgen/regenerate.sh [font.ttf]` (outputs depend on the
  FreeType/Qt version, which is why they are committed instead of built). The `.qkatlas`
  header records the source font's SHA-256 and the Qt version used.
- Source font: DejaVu Sans 2.37 (`DejaVuSans.ttf`), Bitstream Vera licence with
  public-domain DejaVu changes; see `LICENSE.DejaVu`.
