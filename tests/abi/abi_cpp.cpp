// SPDX-License-Identifier: GPL-3.0-or-later
// Exercises the header-only C++ wrapper without dlopen.
#include <qkrellm/plugin.hpp>

#include <cstdio>
#include <stdexcept>

namespace {

int g_logCalls = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables): test probe

void testLog(void* /*ctx*/, qk_log_level /*level*/, const char* /*message*/)
{
    ++g_logCalls;
}

class GoodPlugin
{
public:
    explicit GoodPlugin(const qk_host_api& host) { qk::plugin::Logger(host).info("good: created"); }
};

class ThrowingPlugin
{
public:
    explicit ThrowingPlugin(const qk_host_api& /*host*/) { throw std::runtime_error("boom"); }
};

constinit const qk_plugin_descriptor kGood =
    qk::plugin::makeDescriptor<GoodPlugin>("test.good", "Good", "1.0");
constinit const qk_plugin_descriptor kThrowing =
    qk::plugin::makeDescriptor<ThrowingPlugin>("test.throwing", "Throwing", "1.0");

int fail(const char* what)
{
    std::fprintf(stderr, "abi_cpp: FAIL: %s\n", what);
    return 1;
}

} // namespace

int main()
{
    const qk_host_api host{
        .struct_size = sizeof(qk_host_api),
        .abi_major = QK_ABI_VERSION_MAJOR,
        .abi_minor = QK_ABI_VERSION_MINOR,
        .reserved0 = 0,
        .host_ctx = nullptr,
        .log = &testLog,
    };

    void* instance = nullptr;
    if (kGood.create(&host, &instance) != QK_OK || instance == nullptr) {
        return fail("good create");
    }
    kGood.destroy(instance);
    if (g_logCalls != 1) {
        return fail("logger");
    }

    // An exception inside the plugin constructor must become QK_ERROR, not cross the ABI.
    instance = &g_logCalls; // any non-null sentinel
    if (kThrowing.create(&host, &instance) != QK_ERROR || instance != nullptr) {
        return fail("exception leaked or status wrong");
    }

    qk_host_api wrongMajor = host;
    wrongMajor.abi_major = QK_ABI_VERSION_MAJOR + 1;
    if (kGood.create(&wrongMajor, &instance) != QK_ERROR) {
        return fail("incompatible host accepted");
    }

    std::puts("abi_cpp: OK");
    return 0;
}
