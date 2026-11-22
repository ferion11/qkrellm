@AGENTS.md

## Claude Code specifics

- **Project skills** (`.claude/skills/`): `build-test`, `new-monitor`, `new-plugin`,
  `abi-change`, `perf-budget`, `adr`, `run-qkrellm`, `release`. Prefer them over ad-hoc
  steps — they encode the invariants above.
- **Project agents** (`.claude/agents/`): `abi-guardian` (review any diff touching
  `include/qkrellm/` or `src/pluginhost/`), `perf-reviewer` (review hot-path, painting or
  dependency changes), `linux-sensors` (write parsers + fixtures).
- **Before committing:** run `/code-review` on the diff; `/security-review` when touching
  plugin loading or file parsing. `/simplify` for cleanup passes.
- **Hooks** (`.claude/settings.json`): C/C++ files are clang-formatted after every
  Edit/Write; `cmake` configures that would put a build tree inside the repo are blocked.
- clangd/LSP reads `/tmp/qkrellm/build/dev/compile_commands.json` (see `.clangd`); run
  `cmake --preset dev` first if diagnostics are missing.
- Do not push or open PRs without the maintainer asking.
