# LTE Console

## Overview

The LTE Console application implements a terminal interface that allows interaction with the modem via terminal commands that utilize the HL7800 driver APIs.

HL7800 related commands start with the `hl` command via the UART console. Typing `hl` and then pressing `tab` will show available commands.

Note that this application is intended to demonstrate the functionality of the modem driver. Using AT commands to control the HL7800 from a user application or terminal in Zephyr is not recommended.
The HL7800 driver is intended to provide a networking interface to the user. The user application should use the Zephyr networking APIs (Sockets, CoAP, LwM2M, MQTT) to interact with the modem.

## Configuration Options

The terminal communications parameters are defined in the Pinnacle DVK Boards file. These can be changed by creating an overlay file within the demo app project.
Refer to [Device Tree](https://docs.zephyrproject.org/latest/guides/dts/intro.html#devicetree-intro) and [Device Tree Overlays](https://docs.zephyrproject.org/latest/guides/dts/howtos.html#set-devicetree-overlays) for further details.

## Requirements

A Pinnacle 100 Development Kit or MG100 with SIM Card is needed for this application to be executed.

## Building and Running

From the directory where the `west init` and `west update` commands were issued, the following commands
are used to build the application.

```
west build -p auto -b pinnacle_100_dvk -d Pinnacle_100_Sample_Applications/build/apps/lte_console Pinnacle_100_Sample_Applications/apps/lte_console
```
