## What and why

<!-- One paragraph. Link the roadmap item / issue / ADR. -->

## Definition of Done (AGENTS.md)

- [ ] `cmake --workflow --preset dev` passes with zero warnings
- [ ] `cmake --workflow --preset asan` passes
- [ ] `scripts/lint.sh` clean
- [ ] Parser changes: fixtures + unit test (+ fuzzer)
- [ ] Plugin ABI changes: `include/qkrellm/AGENTS.md` checklist; reviewed by `abi-guardian`
- [ ] `docs/roadmap.md`, `CHANGELOG.md` updated; ADR added if a decision was made
- [ ] No build artefacts in the diff (outputs live in /tmp/qkrellm)

## Resource budget

<!-- Paste `benchmarks/measure.sh` output (release build) for changes to sampling,
painting, startup or dependencies; otherwise write "not affected". -->
