/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include "oob_common.h"
#include "led.h"
#include "oob_ble.h"

/* This function is used to send the latest sensor data out to the server */
void SendData(struct k_work *work);

/* This is a callback function which is called when the send data timer has expired */
void SendDataTimerExpired(struct k_timer *dummy);

s8_t LastTemperatureInt = 0;
u8_t LastTemperatureDec = 0;

s8_t LastHumidityInt = 0;
u8_t LastHumidityDec = 0;

u32_t LastPressure = 0;

bool bUpdatedTemperature = false;
bool bUpdatedHumidity = false;
bool bUpdatedPressure = false;


K_WORK_DEFINE(WorkObject, SendData);
K_TIMER_DEFINE(SendDataTimer, SendDataTimerExpired, NULL);


/* This function is used to send the latest sensor data out to the server */
void SendData(struct k_work *work)
{
}

/* This is a callback function which is called when the send data timer has expired */
void SendDataTimerExpired(struct k_timer *dummy)
{
	if (bUpdatedTemperature == true && bUpdatedHumidity == true && bUpdatedPressure == true)
	{
		//All sensor readings have been received
		k_work_submit(&WorkObject);
		bUpdatedTemperature = false;
		bUpdatedHumidity = false;
		bUpdatedPressure = false;
	}
}

/* This is a callback function which receives sensor readings */
void SensorUpdated(u8_t sensor, s32_t reading)
{
	if (sensor == SENSOR_TYPE_TEMPERATURE)
	{
		//Divide by 100 to get xx.xxC format
		LastTemperatureInt = reading/100;
		LastTemperatureDec = reading-(LastTemperatureInt*100);
		bUpdatedTemperature = true;
	}
	else if (sensor == SENSOR_TYPE_HUMIDITY)
	{
		//Divide by 100 to get xx.xx% format
		LastHumidityInt = reading/100;
		LastHumidityDec = reading-(LastHumidityInt*100);
		bUpdatedHumidity = true;
	}
	else if (sensor == SENSOR_TYPE_PRESSURE)
	{
		//Divide by 10 to get x.xPa format
		LastPressure = reading/10;
		bUpdatedPressure = true;
	}
}

void main(void)
{
	/* init LEDS */
	led_init();
	
	/* Start up BLE portion of the demo */
	oob_ble_initialise();
	oob_ble_set_callback(SensorUpdated);

	/* Start the recurring data sending timer */
	k_timer_start(&SendDataTimer, K_SECONDS(DATA_SEND_TIME), K_SECONDS(DATA_SEND_TIME));
}
