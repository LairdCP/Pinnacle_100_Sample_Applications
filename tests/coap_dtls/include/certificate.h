/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define CA_TAG 1
#define DEVICE_CERT_TAG 2

static const unsigned char ca_certificate[] = {
#include "laird-dev-nbiot-caTrustStore.der.inc"
};

static const unsigned char dev_certificate[] = {
#include "pinnacle_test_cert.der.inc"
};

static const unsigned char dev_key[] = {
#include "pinnacle_test_key_nopass.der.inc"
};

#endif /* __CERTIFICATE_H__ */
