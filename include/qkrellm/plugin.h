/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * qkrellm plugin ABI — public C interface.
 *
 * STATUS: v0 DRAFT. Unstable until milestone M4 freezes ABI v1.
 *
 * Rules (enforced by tests/abi and include/qkrellm/AGENTS.md):
 *  - Pure C11, no Qt, no C++ types. Plugins never link Qt; the host does all drawing.
 *  - Every struct starts with `uint32_t struct_size` and is append-only: never reorder,
 *    remove or change the type of an existing field. Readers must check QK_HAS_FIELD
 *    before touching a field added in a later minor version.
 *  - Adding a field/function  -> bump QK_ABI_VERSION_MINOR.
 *  - Any incompatible change  -> bump QK_ABI_VERSION_MAJOR (the host refuses other majors).
 *  - Exceptions, longjmp and Qt objects must never cross this boundary.
 */
#ifndef QKRELLM_PLUGIN_H
#define QKRELLM_PLUGIN_H

#include <stddef.h>
#include <stdint.h>

/* This is a C ABI: macros, unscoped enums, snake_case and (void) are intentional. */
/* NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum,cppcoreguidelines-use-enum-class,readability-identifier-naming,modernize-redundant-void-arg,modernize-use-using)
 */

#define QK_ABI_VERSION_MAJOR 0u
#define QK_ABI_VERSION_MINOR 1u

/* Name of the single symbol a plugin shared object must export. */
#define QK_PLUGIN_ENTRY_NAME "qk_plugin_entry"

#if defined(__GNUC__) || defined(__clang__)
#define QK_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#define QK_PLUGIN_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum qk_status { QK_OK = 0, QK_ERROR = 1, QK_UNSUPPORTED = 2 } qk_status;

typedef enum qk_log_level {
    QK_LOG_DEBUG = 0,
    QK_LOG_INFO = 1,
    QK_LOG_WARNING = 2,
    QK_LOG_ERROR = 3,
} qk_log_level;

/* Services the host offers to a plugin. Owned by the host; valid until destroy(). */
typedef struct qk_host_api
{
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t reserved0; /* explicit padding; must be 0 */
    void* host_ctx;     /* opaque, pass back to every callback */
    void (*log)(void* host_ctx, qk_log_level level, const char* message);
} qk_host_api;

/* What a plugin exposes. Must have static storage duration inside the plugin. */
typedef struct qk_plugin_descriptor
{
    uint32_t struct_size;
    uint32_t abi_major;  /* QK_ABI_VERSION_MAJOR the plugin was built against */
    uint32_t abi_minor;  /* QK_ABI_VERSION_MINOR the plugin was built against */
    uint32_t reserved0;  /* explicit padding; must be 0 */
    const char* id;      /* stable reverse-DNS id, e.g. "io.github.example.hello" */
    const char* name;    /* human readable, UTF-8 */
    const char* version; /* plugin's own version string */
    qk_status (*create)(const qk_host_api* host, void** out_instance);
    void (*destroy)(void* instance);
} qk_plugin_descriptor;

typedef const qk_plugin_descriptor* (*qk_plugin_entry_fn)(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* True when the (possibly older/newer) struct pointed to by `ptr` contains `field`. */
#define QK_HAS_FIELD(ptr, type, field)                                                             \
    ((ptr)->struct_size >= offsetof(type, field) + sizeof(((type*)0)->field))

/* Defines the exported entry point returning `descriptor_ptr`. Use once per plugin. */
#ifdef __cplusplus
#define QK_DECLARE_PLUGIN_ENTRY(descriptor_ptr)                                                    \
    extern "C" QK_PLUGIN_EXPORT const qk_plugin_descriptor* qk_plugin_entry(void)                  \
    {                                                                                              \
        return (descriptor_ptr);                                                                   \
    }
#else
#define QK_DECLARE_PLUGIN_ENTRY(descriptor_ptr)                                                    \
    QK_PLUGIN_EXPORT const qk_plugin_descriptor* qk_plugin_entry(void);                            \
    QK_PLUGIN_EXPORT const qk_plugin_descriptor* qk_plugin_entry(void)                             \
    {                                                                                              \
        return (descriptor_ptr);                                                                   \
    }
#endif

/* NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-macro-to-enum,modernize-macro-to-enum,cppcoreguidelines-use-enum-class,readability-identifier-naming,modernize-redundant-void-arg,modernize-use-using)
 */

#endif /* QKRELLM_PLUGIN_H */
