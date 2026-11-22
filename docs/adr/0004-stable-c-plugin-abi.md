# 0004 — Stable versioned C plugin ABI with header-only C++ wrapper

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

GKrellM's plugin API exposed GTK internals, so plugins broke with every toolkit change.
A Qt-based C++ plugin interface (`QPluginLoader` + `Q_DECLARE_INTERFACE`) would bind
plugins to the exact Qt version and compiler ABI.

## Decision

- The plugin interface is a **C11 header**, `include/qkrellm/plugin.h`, with no Qt and no
  C++ types. Plugins are shared objects exporting one symbol, `qk_plugin_entry`.
- **Plugins never link Qt.** They provide data and declare panels/metrics; the host draws.
- ABI versioning is separate from the app version: `QK_ABI_VERSION_MAJOR` / `MINOR`.
  Structs begin with `uint32_t struct_size` and are append-only; readers use
  `QK_HAS_FIELD`. Minor = additive; major = breaking; the host rejects other majors.
- `include/qkrellm/plugin.hpp` is an optional header-only C++20 layer (descriptor builder,
  logger, exception firewall) that adds no ABI of its own.
- Until M4 the ABI is **v0 (unstable)**; M4 freezes v1.

## Consequences

- Plugins can be written in C, C++, Rust, Zig… and survive Qt upgrades.
- Rich plugin UIs need host-side drawing primitives designed into the ABI (M4 work).
- Enforced by `tests/abi/` (C11 consumer with layout `_Static_assert`s and `dlopen`,
  C++ wrapper test, and a check that a C plugin pulls in neither Qt nor libstdc++).
- Licensing of the header for non-GPL plugins is an open question (`docs/plugin-api.md`).
