# This file is part of KDUtils.
#
# SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Paul Lemire <paul.lemire@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

# doctest library
fetchcontent_declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.9
)
fetchcontent_makeavailable(doctest)

target_include_directories(
    doctest
    INTERFACE $<BUILD_INTERFACE:${doctest_SOURCE_DIR}/doctest>
              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/KDUtils/doctest>
)

if(APPLE)
    target_compile_options(doctest INTERFACE -Wno-deprecated-declarations)
endif()

install(
    DIRECTORY ${doctest_SOURCE_DIR}/doctest/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/KDUtils/doctest
)

add_subdirectory(auto/utils)
add_subdirectory(auto/foundation)
add_subdirectory(auto/gui)
