/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

// #define USE_DNS

#define SERVER_HOST "nbiot.sparassidae.com"
#define SERVER_PORT_STR "5684"

#ifdef USE_DNS
#define DNS_RETRIES 3
#define DNS_RETRY_TIME_SECONDS 2
#else
#define SERVER_IP_ADDR "54.86.200.135"
#endif /* USE_DNS */

#endif /* __CONFIG_H__ */