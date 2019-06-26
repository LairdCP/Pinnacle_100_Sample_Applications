cmake_minimum_required(VERSION 3.13.1)
set(BOARD_ROOT $ENV{ZEPHYR_BASE}/..)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(tcp_echo_client)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

