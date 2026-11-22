# Public plugin ABI — rules for editing this directory

Everything here is a **public contract** consumed by separately compiled plugins
(ADR 0004, `docs/plugin-api.md`). A mistake here breaks plugins silently at runtime.

## plugin.h (C11)

- Must compile as C11 (`-std=c11 -Wpedantic`) and as C++20. No Qt, no C++ types, no
  `bool` from `<stdbool.h>` in structs (use `uint32_t` flags), no bit-fields, no
  `long`/`size_t` in structs (use fixed-width types), no enums as struct field types
  (store as `uint32_t`; enums are for values only).
- Every struct starts with `uint32_t struct_size`. **Append only**: never reorder, remove,
  rename-with-new-meaning or retype a field. Keep explicit padding (`reserved*`) so layout
  is identical on all compilers.
- Adding a field / function pointer / enum value → bump `QK_ABI_VERSION_MINOR`.
  Anything incompatible → bump `QK_ABI_VERSION_MAJOR` **and write an ADR**.
- Readers must guard new fields with `QK_HAS_FIELD`.
- While the ABI is v0 (before M4) breaking changes are allowed but still bump MAJOR/MINOR
  and update the layout asserts.

## plugin.hpp (C++20 wrapper)

- Header-only, adds no ABI. Must not throw across the C boundary — keep `noexcept` and
  the `try/catch` firewall in `detail::create`.

## Checklist for any change here (the `abi-change` skill automates it)

1. Update `_Static_assert` layout pins in `tests/abi/abi_c11.c`.
2. Extend `tests/abi/abi_cpp.cpp` and/or `plugins/example-c/hello.c` to exercise the change.
3. `ctest --preset dev -R abi` and `cmake --workflow --preset asan` pass.
4. Update `docs/plugin-api.md` (contract + versioning table) and `CHANGELOG.md`.
5. Ask the `abi-guardian` agent to review the diff.
