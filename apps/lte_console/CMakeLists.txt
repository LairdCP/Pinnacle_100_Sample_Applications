# Copyright (c) 2020 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Prepend the additional boards directory but still allow the original Zephyr
# boards directory to be used with lower priority
list(PREPEND BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(LTE_Console)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ${CMAKE_SOURCE_DIR}/../../common/src/led.c
)

target_sources_ifdef(CONFIG_MODEM_HL7800 app PRIVATE
  ${CMAKE_SOURCE_DIR}/../../common/src/lte.c
)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../../common/include)

# Include git info in build
# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Git current branch: ${GIT_BRANCH}")
message(STATUS "Git commit hash: ${GIT_COMMIT_HASH}")
message(STATUS "Generating version.h")

configure_file(
  ${CMAKE_SOURCE_DIR}/include/version.h.in
  ${CMAKE_BINARY_DIR}/generated/version.h
)

include_directories(${CMAKE_BINARY_DIR}/generated)
# END Include git info in build
