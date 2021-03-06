# Copyright (c) 2020 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_connect)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/certs/AmazonRootCA1.der
    ${gen_dir}/RootCA.der.inc
    )

    
generate_inc_file_for_target(
    app
    src/certs/74e10f78bc_dev_cert.der
    ${gen_dir}/device_cert.der.inc
    )


generate_inc_file_for_target(
    app
    src/certs/74e10f78bc_dev_private_key.der
    ${gen_dir}/device_private_key.der.inc
    )
