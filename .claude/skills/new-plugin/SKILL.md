---
name: new-plugin
description: Scaffold a new qkrellm plugin (C or C++) against the public ABI in include/qkrellm, build it with CMake and load-test it. Use when creating an example or third-party plugin, or when testing the plugin ABI from the consumer side.
---

# Create a plugin

Plugins depend **only** on `include/qkrellm/` — never on `src/` — and never link Qt (ADR 0004).
The ABI is v0 (unstable) until M4.

## In-tree example plugin

1. Copy `plugins/example-c/` to `plugins/<name>/`; rename the target and `OUTPUT_NAME`.
   Keep `AUTOMOC OFF` and `PREFIX ""` for C plugins (otherwise Qt's autogen injects a C++
   TU and libstdc++).
2. Give it a unique reverse-DNS `id` and its own `version`.
3. `add_subdirectory(<name>)` in `plugins/CMakeLists.txt`.
4. C++ variant: use `qk::plugin::makeDescriptor<T>()` from `plugin.hpp` (exception firewall)
   and `QK_DECLARE_PLUGIN_ENTRY(&descriptor)`; see `docs/plugin-api.md`.

## Verify

```sh
cmake --build --preset dev
/tmp/qkrellm/build/dev/tests/abi/abi_c11 /tmp/qkrellm/build/dev/plugins/<name>/<output>.so
scripts/check-qt-modules.sh /tmp/qkrellm/build/dev/plugins/<name>/<output>.so --forbid-cxx   # C plugins
nm -D --defined-only <plugin.so>      # only qk_plugin_entry should be exported
```
Optionally register a ctest like `abi.c11_load_example` for it.
