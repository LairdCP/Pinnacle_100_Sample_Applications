#######################
Pinnacle 100: Low Power
#######################

Overview
********

The Low Power demo application demonstrates usage of Zephyr's in-built power management functionality in conjunction 
with the Pinnacle 100 modem. An HTTP GET request is performed on a remote service, after which the system is placed
in low power mode. During this time, the UART used for terminal communications is disabled to minimise power consumption.

Upon expiry of the time defined by SLEEP_TIME_SECONDS, the terminal UART is enabled again, and the HTTP GET repeated.
This behaviour is repeated cyclically until the application is exited.

Configuration Options
*********************

The URL and associated port of the server to process the HTTP GET request is defined by HTTP_HOST and HTTP_PORT in 
the http_get.c file.

Requirements
************

A Pinnacle 100 Development Kit with SIM Card is needed for this application to be executed.

Building and Running
********************

From the directory where the `west init` and `west update` commands were issued, the following commands 
are used to build the application.

Windows
=======
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\low_power pinnacle_100_sample_applications\apps\low_power

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/low_power pinnacle_100_sample_applications/apps/low_power
