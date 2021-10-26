# Low Power

## Overview

The Low Power demo application demonstrates usage of Zephyr's in-built power management functionality in conjunction
with the Pinnacle 100 modem. An HTTP GET request is performed on a remote service, after which the system is placed
in low power mode. During this time, the UART used for terminal communications is disabled to minimise power consumption.

Upon expiry of the time defined by SLEEP_TIME_SECONDS, the terminal UART is enabled again, and the HTTP GET repeated.
This behaviour is repeated forever.

Every 3rd loop, the app switches between keeping the HTTP connection alive and closing it after each request.
This demonstrates it is possible to keep a TCP connection open while using eDRX.

The .sal files in the [docs](./docs) folder can be opened with the [Saleae v2.x Logic software](https://www.saleae.com/downloads/). The logic traces show relevant signals between the nRF52840 and the HL7800 when running this demo in eDRX and PSM modes.

## Configuration Options

The URL and associated port of the server to process the HTTP GET request is defined by `HTTP_HOST` and `HTTP_PORT` in
the [http_get.c](src/http_get.c).

To switch between eDRX and PSM, see [prj.conf](prj.conf).

## Requirements

A Pinnacle 100 Development Kit or MG100 with SIM Card is needed for this application to be executed.

## Building and Running

From the directory where the `west init` and `west update` commands were issued, the following commands
are used to build the application.

```
west build -b pinnacle_100_dvk -d Pinnacle_100_Sample_Applications/build/apps/low_power Pinnacle_100_Sample_Applications/apps/low_power
```
