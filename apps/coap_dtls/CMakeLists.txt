# Copyright (c) 2020 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_dtls)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/net/ip)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_inc_file_for_target(
    app
    src/certs/root_server_cert.der
    ${gen_dir}/root_server_cert.der.inc
    )

generate_inc_file_for_target(
    app
    src/certs/client_cert.der
    ${gen_dir}/client_cert.der.inc
    )

generate_inc_file_for_target(
    app
    src/certs/client_privkey.der
    ${gen_dir}/client_privkey.der.inc
    )
