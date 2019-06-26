/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_main);

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include "oob_common.h"
#include "led.h"
#include "oob_ble.h"
#include "lte.h"

#define MAIN_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define MAIN_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define MAIN_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define MAIN_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

/* This is a callback function which is called when the send data timer has expired */
void SendDataTimerExpired(struct k_timer *dummy);

K_TIMER_DEFINE(SendDataTimer, SendDataTimerExpired, NULL);
K_SEM_DEFINE(send_data_sem, 0, 1);
K_MUTEX_DEFINE(sensor_data_lock);

float temperatureReading = 0;
float humidityReading = 0;
u32_t pressureReading = 0;

bool bUpdatedTemperature = false;
bool bUpdatedHumidity = false;
bool bUpdatedPressure = false;

/* This function is used to send the latest sensor data out to the server */
void SendData(void)
{
	int rc;
	k_mutex_lock(&sensor_data_lock, K_FOREVER);
	MAIN_LOG_INF("Sending Sensor data...");
	rc = lteSendSensorData(temperatureReading, humidityReading,
			       pressureReading);
	if (rc != 0) {
		MAIN_LOG_ERR("Could not send sensor data (%d)", rc);
		led_flash_red();
	} else {
		MAIN_LOG_INF("Data sent");
		led_flash_green();
	}

	k_mutex_unlock(&sensor_data_lock);
}

/* This is a callback function which is called when the send data timer has expired */
void SendDataTimerExpired(struct k_timer *dummy)
{
	if (bUpdatedTemperature == true && bUpdatedHumidity == true &&
	    bUpdatedPressure == true) {
		bUpdatedTemperature = false;
		bUpdatedHumidity = false;
		bUpdatedPressure = false;
		//All sensor readings have been received
		k_sem_give(&send_data_sem);
	}
}

/* This is a callback function which receives sensor readings */
void SensorUpdated(u8_t sensor, s32_t reading)
{
	k_mutex_lock(&sensor_data_lock, K_FOREVER);
	if (sensor == SENSOR_TYPE_TEMPERATURE) {
		//Divide by 100 to get xx.xxC format
		temperatureReading = reading / 100.0;
		bUpdatedTemperature = true;
	} else if (sensor == SENSOR_TYPE_HUMIDITY) {
		//Divide by 100 to get xx.xx% format
		humidityReading = reading / 100.0;
		bUpdatedHumidity = true;
	} else if (sensor == SENSOR_TYPE_PRESSURE) {
		//Divide by 10 to get x.xPa format
		pressureReading = reading / 10;
		bUpdatedPressure = true;
	}
	k_mutex_unlock(&sensor_data_lock);
}

void main(void)
{
	/* init LEDS */
	led_init();

	/* Start up BLE portion of the demo */
	oob_ble_initialise();
	oob_ble_set_callback(SensorUpdated);

	/* Start the recurring data sending timer */
	k_timer_start(&SendDataTimer, K_SECONDS(DATA_SEND_TIME),
		      K_SECONDS(DATA_SEND_TIME));

	while (true) {
		k_sem_take(&send_data_sem, K_FOREVER);
		SendData();
	}
}
