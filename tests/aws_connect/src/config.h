/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define SERVER_HOST "a3273rvo818l4w-ats.iot.us-east-1.amazonaws.com"

#ifdef CONFIG_MQTT_LIB_TLS
#define SERVER_PORT_STR "8883"
#else
#define SERVER_PORT_STR "1883"
#endif

#define APP_SLEEP_MSECS 500

#define DNS_RETRIES 3
#define APP_CONNECT_TRIES 3

#define APP_PUBLISH_INTERVAL_SECS 60
#define APP_PUBLISH_TRIES 1

#define APP_MQTT_BUFFER_SIZE 128

#define MQTT_CLIENTID "zephyr_publisher"

#endif /* __CONFIG_H__ */