# 0012 — Autoscale rate charts to a power-of-two ceiling

- **Status:** Accepted
- **Date:** 2026-11-23
- **Deciders:** maintainer + agent session

## Context

Until M2 every chart showed fractions (0..1: CPU, memory). Network and disk charts show
rates (bytes/s) with no natural maximum: a link may idle at a few hundred B/s and then
burst to tens of MiB/s. A fixed scale makes one of the two invisible; the chart needs a
scale that follows the data, and the user must be able to read what full height means.
Constraints: painting stays cheap (no allocation, one pass over the visible columns), the
scale must not jitter every second, and the label must fit a 70 px panel.

## Decision

- `ChartWidget` has a scale mode: `Fraction` (fixed 0..1, unchanged) or `Auto`.
- `Auto`: at each paint, take the maximum **stacked** value over the columns that are
  visible, and set full height to the smallest power of two (bytes) that is ≥ that
  maximum, with a floor of 1 KiB. Bars are value / scale; the 50 % guide line stays.
- The scale is drawn in the chart's top-left corner with `formatBytes` (e.g. "64K",
  "2.0M"), in the dim text colour; charts without a text renderer skip the label.
- Powers of two make the label exact with the binary units of `formatBytes`, and a burst
  must double the traffic before the scale changes again, which damps jitter. When the
  burst scrolls out of the visible window the scale drops back.

## Consequences

- Up to 2× headroom: a chart may be at most half used at its peak; accepted for a stable,
  exactly-labelled scale.
- Scale depends on the widget width (visible columns), not on the whole history; a wider
  panel can show an older peak — intended.
- Pixel tests in `tst_panels` pin the rule (floor, rounding up, stacking). Revisit if
  users ask for fixed per-interface maxima (M3 config) or a logarithmic scale.
