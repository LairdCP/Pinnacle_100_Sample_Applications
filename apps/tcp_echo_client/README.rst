#############################
Pinnacle 100: TCP Echo Client
#############################

Overview
********

This demo application establishes a connection to a TCP echo server. Upon successful connection, the application then cyclically performs
related operations (e.g. sending data and inspecting the response, disconnecting then reconnecting, etc.).

The application exits only upon occurence of a fatal error.

Configuration Options
*********************

The IP address and associated port of the echo server are configured via the SERVER_IP_ADDR and SERVER_PORT definitions in the main.c file. 
These need to be set to the appropriate values for the echo server being used.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\tcp_echo_client pinnacle_100_sample_applications\apps\tcp_echo_client

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/tcp_echo_client pinnacle_100_sample_applications/apps/tcp_echo_client
