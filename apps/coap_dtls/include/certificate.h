/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

static const unsigned char ca_certificate[] = {
#include "root_server_cert.der.inc"
};

static const unsigned char dev_certificate[] = {
#include "client_cert.der.inc"
};

static const unsigned char dev_key[] = {
#include "client_privkey.der.inc"
};

#endif /* __CERTIFICATE_H__ */
