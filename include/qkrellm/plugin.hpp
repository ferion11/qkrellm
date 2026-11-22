// SPDX-License-Identifier: GPL-3.0-or-later
// Header-only C++20 convenience layer over the C ABI in plugin.h.
// It adds no ABI of its own: everything here compiles down to plugin.h types, so a plugin
// built with this header is binary-identical in contract to one written in plain C.
#pragma once

#include "qkrellm/plugin.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <string_view>

namespace qk::plugin {

inline constexpr std::uint32_t kAbiMajor = QK_ABI_VERSION_MAJOR;
inline constexpr std::uint32_t kAbiMinor = QK_ABI_VERSION_MINOR;

/// True when `host` speaks the same ABI major version this plugin was built against.
[[nodiscard]] constexpr bool isCompatible(const qk_host_api& host) noexcept
{
    return host.struct_size >= sizeof(std::uint32_t) * 3 && host.abi_major == kAbiMajor;
}

/// Thin RAII-free logger bound to the host callbacks.
class Logger
{
public:
    explicit constexpr Logger(const qk_host_api& host) noexcept
        : m_host(&host)
    {}

    void log(qk_log_level level, const char* message) const noexcept
    {
        if (m_host->log != nullptr) {
            m_host->log(m_host->host_ctx, level, message);
        }
    }
    void info(const char* message) const noexcept { log(QK_LOG_INFO, message); }
    void warning(const char* message) const noexcept { log(QK_LOG_WARNING, message); }

private:
    const qk_host_api* m_host;
};

namespace detail {

// Exceptions must never cross the C boundary: catch everything here.
template<typename Impl>
qk_status create(const qk_host_api* host, void** outInstance) noexcept
{
    if (host == nullptr || outInstance == nullptr || !isCompatible(*host)) {
        return QK_ERROR;
    }
    try {
        *outInstance = new Impl(*host);
        return QK_OK;
    } catch (...) {
        *outInstance = nullptr;
        return QK_ERROR;
    }
}

template<typename Impl>
void destroy(void* instance) noexcept
{
    delete static_cast<Impl*>(instance);
}

} // namespace detail

/// Builds a descriptor for a C++ class `Impl` constructible from `const qk_host_api&`.
/// Store the result in a namespace-scope `constinit` variable and pass its address to
/// QK_DECLARE_PLUGIN_ENTRY.
template<typename Impl>
[[nodiscard]] constexpr qk_plugin_descriptor makeDescriptor(const char* id, const char* name,
                                                            const char* version) noexcept
{
    return qk_plugin_descriptor{
        .struct_size = sizeof(qk_plugin_descriptor),
        .abi_major = kAbiMajor,
        .abi_minor = kAbiMinor,
        .reserved0 = 0,
        .id = id,
        .name = name,
        .version = version,
        .create = &detail::create<Impl>,
        .destroy = &detail::destroy<Impl>,
    };
}

} // namespace qk::plugin
