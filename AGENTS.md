# AGENTS.md — instructions for coding agents (and humans)

qkrellm is a GKrellM-spirited system monitor in **Qt 6**, built to be **as light as
possible**, with a **stable C plugin ABI**. Linux only. GPL-3.0-or-later.
Code, comments, docs and commits are in **English**.

**Start every session by reading [`docs/roadmap.md`](docs/roadmap.md)** — it holds the
current status and the "Next steps" left by the previous session.

## Commands

All build, install, test, coverage, lint and benchmark output goes to **`/tmp/qkrellm/`**
(ADR 0010). **Never create build directories, caches or reports inside the repository**;
never run `cmake -B <dir-in-repo>` or `cmake .`. Use the presets.

```sh
cmake --workflow --preset dev          # configure + build + test (Debug, -Werror)
cmake --preset dev && cmake --build --preset dev && ctest --preset dev   # same, step by step
cmake --workflow --preset asan         # ASan + UBSan
cmake --preset release && cmake --build --preset release                 # MinSizeRel + LTO
cmake --install /tmp/qkrellm/build/release --strip   # -> /tmp/qkrellm/install/release
scripts/lint.sh                        # clang-format check, clang-tidy, clazy, cppcheck
scripts/lint.sh fix                    # apply clang-format
benchmarks/measure.sh --check          # resource budget (needs release build)
scripts/coverage.sh                    # -> /tmp/qkrellm/coverage/index.html
ctest --preset dev -R abi              # run a subset
```

Build trees: `/tmp/qkrellm/build/<preset>`; the binary is `…/src/app/qkrellm`.
`/tmp` is a tmpfs: after a reboot just configure again. Tests run with
`QT_QPA_PLATFORM=offscreen`.

## Layout and layering

```
src/app → src/ui → src/pluginhost (M4) → src/core → src/sensors (M1)
include/qkrellm/   public C ABI (plugin.h) + header-only C++ wrapper (plugin.hpp)
plugins/           example plugins — built ONLY against include/qkrellm
tests/{unit,abi,fuzz,fixtures}   benchmarks/   scripts/   docs/{adr,…}   packaging/
```

Dependencies point **down only**. `core` and `sensors` never include QtGui/QtWidgets.
Details: [`docs/architecture.md`](docs/architecture.md).

## Invariants (do not break; each comes from an ADR)

1. **Qt modules: Core, Gui, Widgets only** in the app (ADR 0002; enforced by CTest
   `budget.qt_modules`). Qt ≥ 6.5 APIs only.
2. **No QML / Qt Quick / OpenGL widgets** (ADR 0003). Paint with `QPainter`, repaint only
   dirty regions.
3. **Resource budget** (ADR 0006, `docs/resource-budget.md`): private memory ≤ 16 MiB,
   idle CPU ≤ 0.1 %, startup ≤ 150 ms. **No heap allocation in the sampling hot path.**
4. **Parsers are pure** (`std::string_view` → struct/`std::optional`), each with byte-exact
   fixtures and a unit test (ADR 0005, `src/sensors/AGENTS.md`).
5. **Plugin ABI** in `include/qkrellm/` is C11, append-only, versioned; read
   `include/qkrellm/AGENTS.md` before touching it (ADR 0004).
6. **No GKrellM compatibility** work (themes/gkrellmd) (ADR 0009).
7. **Outputs outside the repo** (ADR 0010). `git status` must be clean after build/test.

Need to break one? Write a new ADR first (`docs/adr/0000-template.md`) and get maintainer
approval.

## Conventions

- Style: `.clang-format` (4 spaces, 100 cols, `Type* p`); a Claude Code hook formats on
  edit, pre-commit formats on commit.
- Naming: namespaces `qk::<layer>`, classes `CamelCase`, members `m_name`, constants
  `kName`, files `lowercase.cpp/.h`. C ABI: `qk_snake_case`, macros `QK_`.
- Headers: `#pragma once`; SPDX line `// SPDX-License-Identifier: GPL-3.0-or-later`.
- Errors: `std::optional`/status returns in core/sensors; no exceptions across ABI.
- Commits: Conventional Commits (`feat(sensors): parse /proc/stat`), one logical change each.
  Branch from `main` (`feat/…`, `fix/…`, `chore/…`); never commit directly to `main`.

## Definition of Done

- [ ] `cmake --workflow --preset dev` passes with zero warnings; `asan` workflow passes
- [ ] `scripts/lint.sh` clean
- [ ] New/changed parser: fixture(s) + unit test (+ fuzzer from M1 on)
- [ ] Budget-relevant change: `benchmarks/measure.sh --check` output in the PR
- [ ] ABI change: `include/qkrellm/AGENTS.md` checklist done
- [ ] Docs updated: `docs/roadmap.md` checkboxes, `CHANGELOG.md` (Unreleased), ADR if a
      decision was made
- [ ] `git status` clean apart from intended changes

## Ending a session (handoff)

Before stopping, update `docs/roadmap.md`: tick finished items, and rewrite **Next steps**
with what the next session should do first, open questions and anything half-done
(branch name, failing test, measurement). Keep it short and concrete.
