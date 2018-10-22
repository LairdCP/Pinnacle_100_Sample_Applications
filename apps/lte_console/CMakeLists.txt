cmake_minimum_required(VERSION 3.8.2)
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(BOARD sirrus_100_dvk)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(LTE_Console)

target_sources(app PRIVATE
  src/main.c
  src/hrs.c
  src/dis.c
  src/bas.c
  src/cts.c
  src/app_bt.c
  src/app_led.c
  src/console_rx.c
  src/hl7800.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
