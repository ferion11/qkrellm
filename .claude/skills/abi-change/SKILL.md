---
name: abi-change
description: Checklist and verification for any change to the qkrellm public plugin ABI (include/qkrellm/plugin.h or plugin.hpp) — versioning, layout asserts, tests, docs and review. Use before and after editing anything in include/qkrellm/.
---

# Changing the plugin ABI

Rules: `include/qkrellm/AGENTS.md`. Rationale: ADR 0004. Contract: `docs/plugin-api.md`.

## 1. Classify the change

| Change | Bump |
|---|---|
| Append field / function pointer at struct end, add enum value | `QK_ABI_VERSION_MINOR` |
| Reorder, remove, retype, change meaning | `QK_ABI_VERSION_MAJOR` + new ADR |

Only fixed-width integer types, pointers and function pointers in structs; explicit padding.

## 2. Edit

- `include/qkrellm/plugin.h` (C11) — new fields go at the end, documented inline with the
  minor version that introduced them, e.g. `/* since 0.2 */`.
- `include/qkrellm/plugin.hpp` — wrapper support, still `noexcept` across the boundary.

## 3. Pin and exercise

- Update the LP64 `_Static_assert` block in `tests/abi/abi_c11.c` (size + offset of every new field).
- Guard reads of new fields with `QK_HAS_FIELD` in host and example code.
- Exercise the feature in `plugins/example-c/hello.c` and `tests/abi/abi_cpp.cpp`.

## 4. Verify

```sh
cmake --workflow --preset dev && ctest --preset dev -R abi -V
cmake --workflow --preset asan
pahole -C qk_host_api /tmp/qkrellm/build/dev/tests/abi/abi_c11 2>/dev/null   # if dwarves is installed
```

## 5. Document and review

- `docs/plugin-api.md` (contract, versioning table), `CHANGELOG.md` under "Plugin ABI".
- Ask the `abi-guardian` agent to review the full diff before committing.
