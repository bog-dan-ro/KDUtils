# This file is part of KDUtils.
#
# SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Paul Lemire <paul.lemire@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

project(
    test-core-win32-platform-event-loop
    VERSION 0.1
    LANGUAGES CXX
)

add_executable(
    ${PROJECT_NAME}
    tst_win32_platform_event_loop.cpp
)

target_link_libraries(
    ${PROJECT_NAME} KDFoundation doctest
)

add_test(NAME ${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}>)
set_tests_properties(${PROJECT_NAME} PROPERTIES LABELS "Core")
