// SPDX-License-Identifier: GPL-3.0-or-later
// Counts calls to fontconfig's FcFontList(), which Qt uses to populate its font database
// (ADR 0011). Include in exactly ONE translation unit of a test executable linked with
// ENABLE_EXPORTS: the definition below then interposes the one in libfontconfig.
#pragma once

#include <atomic>

#include <dlfcn.h>

namespace qk::test {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline std::atomic<int> fcFontListCalls{0};
} // namespace qk::test

// Opaque pointer types stand in for FcFontSet/FcConfig/FcPattern/FcObjectSet.
// Defined in the header on purpose: the interposer must be a strong definition in the test
// executable, and the header is included by exactly one TU per executable.
// NOLINTNEXTLINE(misc-definitions-in-headers)
extern "C" __attribute__((visibility("default"))) void* FcFontList(void* config, void* pattern,
                                                                   void* objectSet)
{
    using Fn = void* (*)(void*, void*, void*);
    qk::test::fcFontListCalls.fetch_add(1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): dlsym returns void*
    static const auto real = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, "FcFontList"));
    return real != nullptr ? real(config, pattern, objectSet) : nullptr;
}
