# SPDX-License-Identifier: GPL-3.0-or-later
# Optional build accelerators. Both degrade gracefully when the tool is missing.

if(QKRELLM_USE_CCACHE AND NOT CMAKE_CXX_COMPILER_LAUNCHER)
    find_program(QK_CCACHE ccache)
    if(QK_CCACHE)
        set(CMAKE_C_COMPILER_LAUNCHER "${QK_CCACHE}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${QK_CCACHE}")
    endif()
endif()

if(QKRELLM_USE_MOLD AND NOT DEFINED CMAKE_LINKER_TYPE)
    find_program(QK_MOLD mold)
    if(QK_MOLD)
        set(CMAKE_LINKER_TYPE MOLD)
    endif()
endif()
