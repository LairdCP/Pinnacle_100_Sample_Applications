#########################
Pinnacle 100: AWS Connect
#########################

Overview
********

The AWS Connect demo application establishes a TCP based connection to an AWS based MQTT Broker and
proceeds to publish to it once connected.

Upon successful resolution of the IP address of the MQTT Broker, the application cyclically publishes
random 32-bit data to the "pinnacle100/data" topic until reset or a fatal error occurs.

Configuration Options
*********************

The CONFIG_MQTT_LIB_TLS option in the config.h file can be used to enable secure connection to the 
MQTT Broker. If disabled, a non-secure connection is used. The appropriate port is defined depending
upon the setting of this option.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\aws_connect pinnacle_100_sample_applications\apps\aws_connect

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/aws_connect pinnacle_100_sample_applications/apps/aws_connect
