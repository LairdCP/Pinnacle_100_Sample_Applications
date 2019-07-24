/* oob_common.h - Header file for the common portions of demo code */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0
#define APP_VERSION_STRING                                                     \
	STRINGIFY(APP_VERSION_MAJOR)                                           \
	"." STRINGIFY(APP_VERSION_MINOR) "." STRINGIFY(APP_VERSION_PATCH)

#define DATA_SEND_TIME_SECONDS 30
#define IMEI_LENGTH 15

#define RETRY_AWS_ACTION_TIMEOUT_SECONDS 30

enum SENSOR_TYPES {
	SENSOR_TYPE_TEMPERATURE = 0,
	SENSOR_TYPE_HUMIDITY,
	SENSOR_TYPE_PRESSURE,
	SENSOR_TYPE_DEW_POINT,

	SENSOR_TYPE_MAX
};

typedef void (*app_state_function_t)(void);