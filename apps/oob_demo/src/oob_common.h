/* oob_common.c - Header file for the common portions of demo code */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define DATA_SEND_TIME	15

enum SENSOR_TYPES
{
	SENSOR_TYPE_TEMPERATURE = 0,
	SENSOR_TYPE_HUMIDITY,
	SENSOR_TYPE_PRESSURE,
	SENSOR_TYPE_DEW_POINT,

	SENSOR_TYPE_MAX
};

/* Callback function for passing sensor data */
typedef void (*sensor_updated_function_t)(u8_t sensor, s32_t reading);
