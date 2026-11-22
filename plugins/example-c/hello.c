/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal plugin in plain C11: proves the public ABI is usable without C++ or Qt. */
#include <qkrellm/plugin.h>

#include <stdlib.h>

typedef struct hello_instance
{
    const qk_host_api* host;
} hello_instance;

static qk_status hello_create(const qk_host_api* host, void** out_instance)
{
    if (host == NULL || out_instance == NULL || host->abi_major != QK_ABI_VERSION_MAJOR) {
        return QK_ERROR;
    }
    hello_instance* self = calloc(1, sizeof *self);
    if (self == NULL) {
        return QK_ERROR;
    }
    self->host = host;
    if (QK_HAS_FIELD(host, qk_host_api, log) && host->log != NULL) {
        host->log(host->host_ctx, QK_LOG_INFO, "example-hello: created");
    }
    *out_instance = self;
    return QK_OK;
}

static void hello_destroy(void* instance)
{
    free(instance);
}

static const qk_plugin_descriptor hello_descriptor = {
    .struct_size = sizeof(qk_plugin_descriptor),
    .abi_major = QK_ABI_VERSION_MAJOR,
    .abi_minor = QK_ABI_VERSION_MINOR,
    .reserved0 = 0,
    .id = "io.github.ferion11.qkrellm.example-hello",
    .name = "Example: hello",
    .version = "0.1.0",
    .create = hello_create,
    .destroy = hello_destroy,
};

QK_DECLARE_PLUGIN_ENTRY(&hello_descriptor)
