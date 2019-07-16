/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __AWS_H__
#define __AWS_H__

#include <json.h>

#define SERVER_HOST "a3273rvo818l4w-ats.iot.us-east-1.amazonaws.com"

#define SERVER_PORT_STR "8883"

#define APP_SLEEP_MSECS 500

#define DNS_RETRIES 3
#define APP_CONNECT_TRIES 3

#define APP_MQTT_BUFFER_SIZE 128

#define MQTT_CLIENTID_PREFIX "pinnacle100_oob_"

#define SHADOW_REPORTED_START "{\"state\":{\"reported\":{"
#define SHADOW_END "}}}"

#define SHADOW_TEMPERATURE "\"temperature\":"
#define SHADOW_HUMIDITY "\"humidity\":"
#define SHADOW_PRESSURE "\"pressure\":"
#define SHADOW_RADIO_RSSI "\"radio_rssi\":"

struct shadow_persistent_values {
	const char *firmware_version;
	const char *os_version;
	const char *radio_version;
	const char *IMEI;
};

struct shadow_state_reported {
	struct shadow_persistent_values reported;
};

struct shadow_reported_struct {
	struct shadow_state_reported state;
};

static const struct json_obj_descr shadow_persistent_values_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct shadow_persistent_values, firmware_version,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct shadow_persistent_values, os_version, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct shadow_persistent_values, radio_version,
			    JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct shadow_persistent_values, IMEI, JSON_TOK_STRING),
};

static const struct json_obj_descr shadow_state_reported_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct shadow_state_reported, reported,
			      shadow_persistent_values_descr),
};

static const struct json_obj_descr shadow_descr[] = {
	JSON_OBJ_DESCR_OBJECT(struct shadow_reported_struct, state,
			      shadow_state_reported_descr),
};

int awsInit(void);
int awsGetServerAddr(void);
int awsConnect(const char *clientId);
void awsDisconnect(void);
int awsKeepAlive(void);
int awsSendData(char *data);
int awsPublishShadowPersistentData();
int awsSetShadowKernelVersion(const char *version);
int awsSetShadowIMEI(const char *imei);
int awsSetShadowRadioFirmwareVersion(const char *version);
int awsSetShadowAppFirmwareVersion(const char *version);

#endif /* __AWS_H__ */
