set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD cell_iot)
set(CMAKE_ECLIPSE_VERSION 4.7)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(Cellular_IOT)

target_sources(app PRIVATE
  src/main.c
  src/hrs.c
  src/dis.c
  src/bas.c
  src/cts.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
