# Contributing to qkrellm

The single source of truth for how to work on this project is **[AGENTS.md](AGENTS.md)** —
commands, layering, invariants, conventions and the Definition of Done. It applies to
humans and AI agents alike.

Quick start:

```sh
pre-commit install                 # once per clone
cmake --workflow --preset dev      # build + test; output in /tmp/qkrellm/build/dev
scripts/lint.sh
```

Requirements: CMake ≥ 3.25, Ninja, a C++20 compiler (GCC ≥ 12 or Clang ≥ 16), Qt ≥ 6.5
(Core, Gui, Widgets, Test). Optional: clang-tidy, clazy, cppcheck, gcovr, heaptrack,
valgrind, mold, ccache.

- Read `docs/roadmap.md` to see what is in progress.
- Architectural changes need an ADR (`docs/adr/`).
- Commits follow [Conventional Commits](https://www.conventionalcommits.org/); branches
  `feat/…`, `fix/…`, `chore/…`, `docs/…`.
- Every PR fills in the template checklist, including budget numbers when relevant.
