# Plugin API

> **Status: v0 draft — unstable until M4 freezes ABI v1.** Do not ship third-party plugins
> against v0.

Design rationale: ADR 0004. Headers: [`include/qkrellm/plugin.h`](../include/qkrellm/plugin.h)
(C11) and [`include/qkrellm/plugin.hpp`](../include/qkrellm/plugin.hpp) (optional C++20).

## Contract

1. A plugin is a shared object exporting exactly one public symbol, `qk_plugin_entry`
   (`QK_PLUGIN_ENTRY_NAME`), returning a pointer to a static `qk_plugin_descriptor`.
2. The host checks `descriptor->abi_major == QK_ABI_VERSION_MAJOR`; anything else is
   refused. `abi_minor` tells which optional fields the plugin knows.
3. The host calls `create(host_api, &instance)`; on `QK_OK` it later calls
   `destroy(instance)` exactly once, on the same thread.
4. Every struct starts with `uint32_t struct_size`. Before reading a field added in a later
   minor version, check `QK_HAS_FIELD(ptr, type, field)`.
5. Strings are UTF-8, owned by whoever provides them, valid for the lifetime of the
   descriptor / host API respectively.
6. Never let C++ exceptions, `longjmp`, or Qt objects cross the boundary.
   `qk::plugin::makeDescriptor<T>()` installs an exception firewall for C++ plugins.
7. Plugins do not link Qt (enforced for the example plugin by the
   `abi.c_plugin_has_no_cxx_or_qt` test).

## Versioning rules

| Change | Version bump | Allowed in v1+? |
|---|---|---|
| Append a field at the end of a struct | MINOR | yes |
| Add a new function pointer to `qk_host_api` (at the end) | MINOR | yes |
| Add an enum value | MINOR | yes (readers must handle unknown values) |
| Reorder/remove/retype a field, change semantics | MAJOR | only with an ADR |

Layout of each version is pinned by `_Static_assert`s in `tests/abi/abi_c11.c`.

## Minimal plugin (C)

See [`plugins/example-c/hello.c`](../plugins/example-c/hello.c).

## Minimal plugin (C++)

```cpp
#include <qkrellm/plugin.hpp>

class Hello {
public:
    explicit Hello(const qk_host_api& host) { qk::plugin::Logger(host).info("hello"); }
};

constinit const qk_plugin_descriptor kHello =
    qk::plugin::makeDescriptor<Hello>("org.example.hello", "Hello", "1.0");
QK_DECLARE_PLUGIN_ENTRY(&kHello)
```

## To design in M4

- Metric declaration (id, unit, range, chart vs. krell vs. text) and value push/pull.
- Panel description so the host can draw plugin UIs without the plugin touching Qt.
- Per-plugin config storage through the host.
- Threading model for plugins that must block (worker thread owned by host?).
- Crash isolation policy (out-of-process plugins? at least `create` timeouts).
- **License question:** the headers are GPL-3.0-or-later today, which makes plugins
  derivative works. Decide whether to add a plugin exception or relicense the two public
  headers (e.g. MIT) before freezing v1.
