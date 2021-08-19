#########################
Pinnacle 100: LTE Console
#########################

Overview
********

The LTE Console application implements a terminal interface that allows interaction with the modem via its published AT command set. It also allows
the power consumption of the modem to be externally observed by triggering sleep and shut down modes of the modem. 

Commands available are as follows.

* pwr_off - Shuts the modem down
* reset - Resets the modem
* wake - Sends the modem to sleep, wakes if asleep
* send - Sends an AT command to the modem, the AT command follows the send command

The application exits only upon occurrence of a fatal error.

Note that this application is intended to demonstrate the functionality of the modem driver. Should an AT interface be required for a production
design, it is recommended to utilise the Hosted_Mode_Firmware_.

Configuration Options
*********************

The terminal communications parameters are defined in the Pinnacle DVK Boards file. These can be changed by creating an overlay file within the demo app project. 
Refer to Device_Tree_ and Device_Tree_Overlays_ for further details.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\lte_console pinnacle_100_sample_applications\apps\lte_console

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/lte_console pinnacle_100_sample_applications/apps/lte_console

References
**********

.. target-notes::

.. _Hosted_Mode_Firmware: https://connectivity-staging.s3.us-east-2.amazonaws.com/2020-04/480-00079%20Pinnacle%20100%20AT%20Hosted%20Mode%20Firmware%20Version%201%20Build%2019.zip
.. _Device_Tree: https://docs.zephyrproject.org/latest/guides/dts/intro.html#devicetree-intro
.. _Device_Tree_Overlays: https://docs.zephyrproject.org/latest/guides/dts/howtos.html#set-devicetree-overlays
