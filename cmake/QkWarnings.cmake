# SPDX-License-Identifier: GPL-3.0-or-later
# qk_set_warnings(<target>) — project-wide warning policy.

function(qk_set_warnings target)
    set(common
        -Wall -Wextra -Wpedantic
        -Wshadow -Wconversion -Wsign-conversion
        -Wcast-align -Wnull-dereference -Wdouble-promotion
        -Wformat=2 -Wimplicit-fallthrough)
    set(cxx_only -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual)

    target_compile_options(${target} PRIVATE
        ${common}
        $<$<COMPILE_LANGUAGE:CXX>:${cxx_only}>)

    if(QKRELLM_WARNINGS_AS_ERRORS)
        target_compile_options(${target} PRIVATE -Werror)
    endif()
endfunction()
