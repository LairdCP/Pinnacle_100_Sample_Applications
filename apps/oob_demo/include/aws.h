/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AWS_H__
#define __AWS_H__

#define SERVER_HOST "a3273rvo818l4w-ats.iot.us-east-1.amazonaws.com"

#define SERVER_PORT_STR "8883"

#define APP_SLEEP_MSECS 500

#define DNS_RETRIES 3
#define APP_CONNECT_TRIES 3

#define APP_MQTT_BUFFER_SIZE 128

#define MQTT_CLIENTID "pinnacle100_oob_demo"

int awsInit(void);
int awsGetServerAddr(void);
int awsConnect(void);
void awsDisconnect(void);
int awsKeepAlive(void);
int awsSendData(u32_t data);

#endif /* __AWS_H__ */
