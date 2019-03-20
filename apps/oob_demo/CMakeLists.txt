cmake_minimum_required(VERSION 3.13.1)
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(BOARD sirrus_100_dvk)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(central)

target_sources(app PRIVATE
  src/main.c
  src/oob_ble.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
