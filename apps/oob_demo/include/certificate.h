/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define CA_TAG 1
#define DEVICE_CERT_TAG 2

static const unsigned char ca_certificate[] = {
#include "RootCA.der.inc"
};

static const unsigned char dev_certificate[] = {
#include "device_cert.der.inc"
};

static const unsigned char dev_key[] = {
#include "device_private_key.der.inc"
};

#endif /* __CERTIFICATE_H__ */
