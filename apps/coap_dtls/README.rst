#######################
Pinnacle 100: COAP DTLS
#######################

Overview
********

The COAP DTLS demo application demonstrates COAP connectivity via the Pinnacle 100 modem. Secure transfer of data is achieved via
DTLS due to COAP being based upon UDP. 

Upon having determined availability of the COAP server, several COAP operations are performed (GET, PUT) and the results checked.

The application exits either a fatal error occuring, or all operations having completed succesfully.

Configuration Options
*********************

The address and associated port of the COAP server to connect to are configured via the SERVER_HOST and 
SERVER_PORT_STR option in the config.h file.

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

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\coap_dtls pinnacle_100_sample_applications\apps\coap_dtls

Linux and macOS
===============
.. code-block::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/coap_dtls pinnacle_100_sample_applications/apps/coap_dtls
