# This is a copy of the pico_sdk_import.cmake file from the Raspberry Pi Pico SDK.
# It is used to find the location of the PICO SDK.
#
# Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# This file should be included by CMake projects that want to use the Raspberry Pi Pico SDK.
# It is responsible for finding the SDK and making its functions and targets available.

if (DEFINED PICO_SDK_PATH AND (NOT IS_ABSOLUTE "${PICO_SDK_PATH}"))
    get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" ABSOLUTE)
    message(STATUS "PICO_SDK_PATH is now ${PICO_SDK_PATH}")
endif ()

if (NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH_GUESS "$ENV{PICO_SDK_PATH}")
    if (PICO_SDK_PATH_GUESS)
        message(STATUS "PICO_SDK_PATH is not set. Trying '$ENV{PICO_SDK_PATH}' from environment.")
        set(PICO_SDK_PATH ${PICO_SDK_PATH_GUESS} CACHE PATH "Path to the Raspberry Pi Pico SDK")
    else()
        # We need to find the SDK (note we are in a sub dir of the project)
        get_filename_component(CURRENT_PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)
        # We search this path, but PICO_SDK_PATH is relative to the CMAKE_SOURCE_DIR
        # This is where we expect the SDK to be installed by default
        set(PICO_SDK_PATH_GUESS "${CURRENT_PROJECT_SOURCE_DIR}/pico/pico-sdk")
        if (EXISTS "${PICO_SDK_PATH_GUESS}/pico_sdk_init.cmake")
            message(STATUS "PICO_SDK_PATH is not set. Found default location at ${PICO_SDK_PATH_GUESS}")
            set(PICO_SDK_PATH ${PICO_SDK_PATH_GUESS} CACHE PATH "Path to the Raspberry Pi Pico SDK")
        else()
            message(FATAL_ERROR "PICO_SDK_PATH not set and no default found at ${PICO_SDK_PATH_GUESS}")
        endif()
    endif ()
endif ()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)

if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' does not appear to be a Pico SDK directory (missing ${PICO_SDK_INIT_CMAKE_FILE})")
endif ()

include(${PICO_SDK_INIT_CMAKE_FILE})
