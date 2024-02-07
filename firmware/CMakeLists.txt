# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
# Copyright (c) 2023 Rene Wolf

cmake_minimum_required(VERSION 3.5)

set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

# Pull in SDK (must be before project)
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(cxadc-clock-generator-audio-adc C CXX ASM)

# Initialize the SDK
pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -Werror=return-type
        )

add_subdirectory("src" "build")
