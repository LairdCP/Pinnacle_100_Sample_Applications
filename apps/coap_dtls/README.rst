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

See the `app Kconfig file <Kconfig>`_ for application configuration options.

Requirements
************

1. A CoAP server
2. A Pinnacle 100 Development Kit with SIM Card is needed for this application to be executed

Setup
*****

CoAP server
===========

> NOTE: CoAP server needs to be accessable via the internet in order for Pinnacle 100 modem to communicate with it.

1. Download and build libcoap_::

        git clone https://github.com/obgm/libcoap.git
        cd libcoap
        ./configure --with-openssl
        make
        make install

2. Generate certificates::

        ./src/certs/gen_certs.sh

3. Combine ``server_cert.pem`` and ``server_privkey.pem``::

        cat server_cert.pem server_privkey.pem > server_cert_combined.pem

4. Start server::

        coap-server -A 172.31.43.206 -v 9 -n -c certs/server_cert_combined.pem -R certs/root_server_cert.pem

Prepare Client certs
====================

::

        openssl x509 -outform der -in root_server_cert.pem -out root_server_cert.der
        openssl x509 -outform der -in client_cert.pem -out client_cert.der
        openssl ec -outform der -in client_privkey.pem -out client_privkey.der

Building and Running
********************

From the directory where the `west init` and `west update` commands were issued, the following commands 
are used to build the application.

Windows
=======
::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications\build\apps\coap_dtls pinnacle_100_sample_applications\apps\coap_dtls

Linux and macOS
===============
::

        west build -b pinnacle_100_dvk -d pinnacle_100_sample_applications/build/apps/coap_dtls pinnacle_100_sample_applications/apps/coap_dtls

.. _libcoap: https://github.com/obgm/libcoap
