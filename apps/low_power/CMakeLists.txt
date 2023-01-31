# Copyright (c) 2020-2023 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pinnacle100_power_mgmt)

target_sources(app PRIVATE
    src/main.c
    ${CMAKE_SOURCE_DIR}/../../common/src/led.c)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../../common/include)

if(CONFIG_NETWORKING)
    target_sources(app PRIVATE
        src/http_get.c)
endif()
