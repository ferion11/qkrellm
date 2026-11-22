# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 REQUIRED COMPONENTS Test)

add_subdirectory(unit)
add_subdirectory(abi)

if(QKRELLM_ENABLE_FUZZ)
    add_subdirectory(fuzz)
endif()

# --- Smoke test: the real binary starts, paints once and exits (offscreen platform). ---
add_test(NAME app.first_paint COMMAND qkrellm --exit-after-first-paint)
set_tests_properties(app.first_paint PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "first-paint-ms=[0-9]+"
    TIMEOUT 20)

# --- Resource budget guard: only allowed Qt modules may be linked (ADR 0002). ---
add_test(NAME budget.qt_modules
    COMMAND "${PROJECT_SOURCE_DIR}/scripts/check-qt-modules.sh"
            "$<TARGET_FILE:qkrellm>" ${QKRELLM_ALLOWED_QT_MODULES})
