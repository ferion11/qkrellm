# Test fixtures

Byte-exact snapshots of kernel interfaces (`/proc`, `/sys`) used by parser unit tests.

Layout: `proc/<tag>/<original absolute path>`, e.g.
`proc/linux-6.18-x86_64-desktop/proc/stat`, plus a `UNAME` file per tag.

Rules:
- Capture with `tests/fixtures/capture.sh <tag>` (writes to `/tmp/qkrellm/scratch/` first).
- **Scrub personal data** (hostnames, MAC addresses, serial numbers, user paths) before committing.
- Never edit a fixture to make a test pass; add a new tag instead.
- Keep at least one old-kernel tag per parser when formats differ between versions.
- Tests locate fixtures through the `QK_FIXTURES_DIR` compile definition.
