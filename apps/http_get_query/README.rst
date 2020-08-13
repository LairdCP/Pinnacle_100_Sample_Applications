############################
Pinnacle 100: HTTP Get Query
############################

Overview
********

This demo application performs an HTTP GET with a query string and inspects the response.

Configuration Options
*********************

The URL and associated port of the server to process the HTTP GET request is defined by HTTP_HOST and HTTP_PORT in 
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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\http_get_query pinnacle_100_sample_applications\apps\http_get_query

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/http_get_query pinnacle_100_sample_applications/apps/http_get_query
