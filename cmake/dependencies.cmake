# This file is part of KDUtils.
#
# SPDX-FileCopyrightText: 2021-2023 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
# Author: Paul Lemire <paul.lemire@kdab.com>
#
# SPDX-License-Identifier: MIT
#
# Contact KDAB at <info@kdab.com> for commercial licensing options.
#

include(FetchContent)

message(STATUS "Looking for KDUtils dependencies")

# spdlog Logging Library
# spdlog needs to be installed. If already exists in the prefix,
# we don't want to override it, so first we try to find it.
# If we don't find it, then we fetch it and install it
find_package(spdlog 1.11.0 QUIET)

if(NOT TARGET spdlog::spdlog)
    get_property(tmp GLOBAL PROPERTY PACKAGES_NOT_FOUND)
    list(
        FILTER
        tmp
        EXCLUDE
        REGEX
        spdlog
    )
    set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND ${tmp})

    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.11.0
    )
    set(SPDLOG_INSTALL
        ON
        CACHE BOOL "Install spdlog" FORCE
    )
    FetchContent_MakeAvailable(spdlog)

    set_target_properties(
        spdlog
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

find_package(spdlog_setup QUIET)

if(NOT TARGET spdlog_setup::spdlog_setup)
    get_property(tmp GLOBAL PROPERTY PACKAGES_NOT_FOUND)
    list(
        FILTER
        tmp
        EXCLUDE
        REGEX
        spdlog
    )
    set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND ${tmp})

    # Patched version of spdlog_setup which doesn't look for spdlog if not needed.
    # Once the patch is accepted upstream, we can revert to use upstream package
    fetchcontent_declare(
        spdlog_setup
        GIT_REPOSITORY https://github.com/jjcasmar/spdlog_setup.git
        GIT_TAG bf46b966ef4b2f4aca67e7b69c64c2d2def65d94
    )
    set(SPDLOG_SETUP_INSTALL
        ON
        CACHE BOOL "Install spdlog_setup" FORCE
    )
    fetchcontent_makeavailable(spdlog_setup)
    add_library(spdlog_setup::spdlog_setup ALIAS spdlog_setup)

    if(UNIX)
        target_compile_options(spdlog_setup INTERFACE -Wno-deprecated-declarations)
        target_compile_options(spdlog_setup INTERFACE -Wno-unqualified-std-cast-call)
    endif()
endif()

# KDBindings library
fetchcontent_declare(
    KDBindings
    GIT_REPOSITORY https://github.com/KDAB/KDBindings.git
    GIT_TAG v1.0.3
    USES_TERMINAL_DOWNLOAD YES USES_TERMINAL_UPDATE YES
)
fetchcontent_makeavailable(KDBindings)

# whereami library
fetchcontent_declare(
    whereami
    GIT_REPOSITORY https://github.com/gpakosz/whereami
    GIT_TAG e4b7ba1be0e9fd60728acbdd418bc7195cdd37e7 # master at 5/July/2021
)
fetchcontent_makeavailable(whereami)
