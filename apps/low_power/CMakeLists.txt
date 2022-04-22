# Copyright (c) 2020 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Prepend the additional boards directory but still allow the original Zephyr
# boards directory to be used with lower priority
list(PREPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pinnacle100_power_mgmt)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
    ${app_sources}
    ${CMAKE_SOURCE_DIR}/../../common/src/lte.c
    ${CMAKE_SOURCE_DIR}/../../common/src/led.c)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../../common/include)
