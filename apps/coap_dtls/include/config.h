/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define SERVER_HOST CONFIG_APP_SERVER_HOSTNAME
#define SERVER_PORT_STR CONFIG_APP_SERVER_PORT

#ifdef CONFIG_DNS_RESOLVER
#define DNS_RETRIES 3
#define DNS_RETRY_TIME_SECONDS 2
#else
#define SERVER_IP_ADDR CONFIG_APP_SERVER_IP_ADDRESS
#endif /* CONFIG_DNS_RESOLVER */

#endif /* __CONFIG_H__ */