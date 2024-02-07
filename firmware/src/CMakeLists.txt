# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
# Copyright (c) 2023 Rene Wolf

cmake_minimum_required(VERSION 3.12)

# https://stackoverflow.com/questions/25072485/cmake-add-all-sources-in-subdirectory-to-cmake-project
aux_source_directory(${CMAKE_CURRENT_LIST_DIR} ALL_SRC)
aux_source_directory("${CMAKE_CURRENT_LIST_DIR}/libsi5351" ALL_SRC)
add_executable(firmware ${ALL_SRC})

# https://stackoverflow.com/questions/2183324/looping-over-files-matching-wildcard-in-cmake
file(GLOB pio_files "${CMAKE_CURRENT_LIST_DIR}/*.pio")
foreach(file ${pio_files})
pico_generate_pio_header(firmware ${file})
endforeach()

target_include_directories(firmware PRIVATE ${CMAKE_CURRENT_LIST_DIR})



target_link_libraries(firmware PRIVATE pico_stdlib pico_multicore pico_unique_id hardware_i2c hardware_uart hardware_pio tinyusb_device tinyusb_board)


pico_add_extra_outputs(firmware)
