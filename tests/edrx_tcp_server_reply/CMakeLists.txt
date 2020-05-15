cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(edrx_tcp_server_reply)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE 
    ${app_sources}
    ${CMAKE_SOURCE_DIR}/../../common/src/lte.c
    ${CMAKE_SOURCE_DIR}/../../common/src/led.c)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../../common/include)
