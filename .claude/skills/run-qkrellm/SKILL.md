---
name: run-qkrellm
description: Launch the qkrellm app to see a change working — headless (offscreen) smoke run, real X11 window, or a screenshot of the window. Use when asked to run, start, try, or screenshot qkrellm, or to confirm a UI change in the real app rather than only in tests.
---

# Run qkrellm

Binary: `/tmp/qkrellm/build/<preset>/src/app/qkrellm` (build first with the `build-test` skill).

## Headless smoke run (always works)

```sh
QT_QPA_PLATFORM=offscreen /tmp/qkrellm/build/dev/src/app/qkrellm --exit-after-first-paint
# prints first-paint-ms=<n> and exits 0
```

## Real window (X11 session, DISPLAY set)

```sh
/tmp/qkrellm/build/dev/src/app/qkrellm &   # remember the PID; kill it when done
```

## Screenshot

Use Qt itself so no extra tools are needed — grab the window from a scratch test or run
with the offscreen platform and `QWidget::grab()` in a unit test (`tests/unit/`, saving to
`/tmp/qkrellm/scratch/*.png`). With an X server, `import -window root` (ImageMagick) or
`xwd -root | convert xwd:- png:/tmp/qkrellm/scratch/shot.png` also work if installed.
Then view the PNG with the Read tool.

Always stop processes you started and write screenshots only under `/tmp/qkrellm/`.
