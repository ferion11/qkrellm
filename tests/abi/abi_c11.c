/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Loads a plugin exactly as the host will (dlopen + dlsym) using only the public C header. */
#include <qkrellm/plugin.h>

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

/* Layout is part of the ABI. If one of these fires, you changed the ABI: read
 * include/qkrellm/AGENTS.md before "fixing" the assertion. */
_Static_assert(offsetof(qk_host_api, struct_size) == 0, "struct_size must be first");
_Static_assert(offsetof(qk_plugin_descriptor, struct_size) == 0, "struct_size must be first");
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
_Static_assert(sizeof(qk_host_api) == 32, "qk_host_api v0.1 layout changed");
_Static_assert(offsetof(qk_host_api, host_ctx) == 16, "qk_host_api v0.1 layout changed");
_Static_assert(offsetof(qk_host_api, log) == 24, "qk_host_api v0.1 layout changed");
_Static_assert(sizeof(qk_plugin_descriptor) == 56, "qk_plugin_descriptor v0.1 layout changed");
_Static_assert(offsetof(qk_plugin_descriptor, id) == 16,
               "qk_plugin_descriptor v0.1 layout changed");
_Static_assert(offsetof(qk_plugin_descriptor, create) == 40,
               "qk_plugin_descriptor v0.1 layout changed");
_Static_assert(offsetof(qk_plugin_descriptor, destroy) == 48,
               "qk_plugin_descriptor v0.1 layout changed");
#endif

static int g_log_calls = 0;

static void test_log(void* ctx, qk_log_level level, const char* message)
{
    (void)ctx;
    (void)level;
    (void)message;
    ++g_log_calls;
}

static int fail(const char* what)
{
    fprintf(stderr, "abi_c11: FAIL: %s\n", what);
    return 1;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        return fail("usage: abi_c11 <plugin.so>");
    }
    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return fail("dlopen");
    }

    /* ISO C forbids casting void* to a function pointer; copy the bits instead (POSIX-blessed). */
    void* symbol = dlsym(handle, QK_PLUGIN_ENTRY_NAME);
    if (symbol == NULL) {
        return fail("entry symbol not exported");
    }
    qk_plugin_entry_fn entry = NULL;
    memcpy((void*)&entry, (const void*)&symbol, sizeof entry);

    const qk_plugin_descriptor* desc = entry();
    if (desc == NULL) {
        return fail("entry returned NULL");
    }
    if (desc->abi_major != QK_ABI_VERSION_MAJOR) {
        return fail("ABI major mismatch");
    }
    if (!QK_HAS_FIELD(desc, qk_plugin_descriptor, destroy)) {
        return fail("descriptor too small");
    }
    if (desc->id == NULL || desc->create == NULL || desc->destroy == NULL) {
        return fail("descriptor incomplete");
    }

    const qk_host_api host = {
        .struct_size = sizeof(qk_host_api),
        .abi_major = QK_ABI_VERSION_MAJOR,
        .abi_minor = QK_ABI_VERSION_MINOR,
        .reserved0 = 0,
        .host_ctx = NULL,
        .log = test_log,
    };
    void* instance = NULL;
    if (desc->create(&host, &instance) != QK_OK || instance == NULL) {
        return fail("create");
    }
    desc->destroy(instance);
    if (g_log_calls != 1) {
        return fail("plugin did not log through the host API");
    }

    /* Descriptor strings live inside the plugin: use them before dlclose(). */
    printf("abi_c11: OK (%s %s)\n", desc->id, desc->version);
    dlclose(handle);
    return 0;
}
