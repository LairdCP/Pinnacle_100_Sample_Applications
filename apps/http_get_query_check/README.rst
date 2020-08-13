##################################
Pinnacle 100: HTTP Get Query Check
##################################

Overview
********

This demo application performs an HTTP POST request. It exits upon having completed the POST or when any error occurs.

Configuration Options
*********************

The URL and associated port of the server to process the HTTP POST request is defined by HTTP_HOST and HTTP_PORT in 
the main.c file.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\http_get_query_check pinnacle_100_sample_applications\apps\http_get_query_check

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/http_get_query_check pinnacle_100_sample_applications/apps/http_get_query_check
