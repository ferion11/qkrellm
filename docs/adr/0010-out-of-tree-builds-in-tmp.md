# 0010 — All local build/test/bench output lives in /tmp/qkrellm

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

The maintainer wants the source tree to contain only source: no build directories,
caches, coverage or benchmark output mixed with project files. `/tmp` is a fast tmpfs on
the development machine.

## Decision

- Every CMake preset uses `binaryDir = /tmp/qkrellm/build/<preset>` and
  `installDir = /tmp/qkrellm/install/<preset>`.
- Scripts write to `${QKRELLM_OUT:-/tmp/qkrellm}` (`lint-db/`, `coverage/`, `bench/`,
  `fuzz/`, `scratch/`, `cppcheck/`).
- `CMakeLists.txt` refuses in-source builds; a Claude Code hook refuses `cmake -B` paths
  inside the repo; pre-commit refuses committed build artefacts.
- `.clangd` points at `/tmp/qkrellm/build/dev` for the compile database.

## Consequences

- After a reboot `/tmp` is empty: run `cmake --preset dev` again (ccache in
  `~/.cache/ccache` keeps rebuilds fast).
- `git status` must stay clean after any build/test/install — part of verification.
