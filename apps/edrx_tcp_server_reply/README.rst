###################################
Pinnacle 100: EDRX TCP Server Reply
###################################

Overview
********

The EDRX TCP Server Reply demo application demonstrates the EDRX power management feature of the Pinnacle 100. 
It establishes a TCP connection with a remote server and attempts to receive data via a socket. Once complete, 
the socket is closed, and the modem put into sleep mode. This is performed cyclically until an error occurs or the 
application is exited.

Configuration Options
*********************

The IP address and associated port of the echo server are configured via the SERVER_IP_ADDR and SERVER_PORT definitions
in the config.h file. These need to be set to the appropriate values for the echo server being used.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\edrx_tcp_server_reply pinnacle_100_sample_applications\apps\edrx_tcp_server_reply

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/edrx_tcp_server_reply pinnacle_100_sample_applications/apps/edrx_tcp_server_reply
