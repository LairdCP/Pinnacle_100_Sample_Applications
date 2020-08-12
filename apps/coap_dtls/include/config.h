/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define SERVER_HOST "nbiot.sparassidae.com"
#define SERVER_PORT_STR "5684"

#ifdef CONFIG_DNS_RESOLVER
#define DNS_RETRIES 3
#define DNS_RETRY_TIME_SECONDS 2
#else
#define SERVER_IP_ADDR "3.89.10.212"
#endif /* CONFIG_DNS_RESOLVER */

#endif /* __CONFIG_H__ */