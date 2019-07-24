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
#include <version.h>

#include "oob_common.h"
#include "led.h"
#include "oob_ble.h"
#include "lte.h"
#include "aws.h"

#define MAIN_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define MAIN_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define MAIN_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define MAIN_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

/* This is a callback function which is called when the send data timer has expired */
static void SendDataTimerExpired(struct k_timer *dummy);

static void appStateAwsSendSenorData(void);
static void appStateAwsInitShadow(void);
static void appStateAwsConnect(void);
static void appStateAwsDisconnect(void);
static void appStateAwsResolveServer(void);
static void appStateWaitForLte(void);

K_TIMER_DEFINE(SendDataTimer, SendDataTimerExpired, NULL);
K_SEM_DEFINE(send_data_sem, 0, 1);
K_MUTEX_DEFINE(sensor_data_lock);
K_SEM_DEFINE(lte_ready_sem, 0, 1);

static float temperatureReading = 0;
static float humidityReading = 0;
static u32_t pressureReading = 0;
static bool initShadow = true;
static bool sendSensorDataAsap = true;
static bool resolveAwsServer = true;

static bool bUpdatedTemperature = false;
static bool bUpdatedHumidity = false;
static bool bUpdatedPressure = false;

static app_state_function_t appState;
struct lte_status *lteInfo;

static void startSendDataTimer(void)
{
	/* Start the recurring data sending timer */
	k_timer_start(&SendDataTimer, K_SECONDS(DATA_SEND_TIME_SECONDS),
		      K_SECONDS(DATA_SEND_TIME_SECONDS));
}

static void stopSendDataTimer(void)
{
	k_timer_stop(&SendDataTimer);
}

/* This is a callback function which is called when the send data timer has expired */
static void SendDataTimerExpired(struct k_timer *dummy)
{
	if (bUpdatedTemperature == true && bUpdatedHumidity == true &&
	    bUpdatedPressure == true) {
		bUpdatedTemperature = false;
		bUpdatedHumidity = false;
		bUpdatedPressure = false;
		// All sensor readings have been received
		k_sem_give(&send_data_sem);
	}
}

/* This is a callback function which receives sensor readings */
static void SensorUpdated(u8_t sensor, s32_t reading)
{
	k_mutex_lock(&sensor_data_lock, K_FOREVER);
	if (sensor == SENSOR_TYPE_TEMPERATURE) {
		// Divide by 100 to get xx.xxC format
		temperatureReading = reading / 100.0;
		bUpdatedTemperature = true;
	} else if (sensor == SENSOR_TYPE_HUMIDITY) {
		// Divide by 100 to get xx.xx% format
		humidityReading = reading / 100.0;
		bUpdatedHumidity = true;
	} else if (sensor == SENSOR_TYPE_PRESSURE) {
		// Divide by 10 to get x.xPa format
		pressureReading = reading / 10;
		bUpdatedPressure = true;
	}
	k_mutex_unlock(&sensor_data_lock);
	if (sendSensorDataAsap && bUpdatedTemperature && bUpdatedHumidity &&
	    bUpdatedPressure) {
		sendSensorDataAsap = false;
		k_sem_give(&send_data_sem);
		startSendDataTimer();
	}
}

static void lteEvent(enum lte_event event)
{
	switch (event) {
	case LTE_EVT_READY:
		k_sem_give(&lte_ready_sem);
		break;
	case LTE_EVT_DISCONNECTED:
		/* No need to trigger a reconnect.
		*  If next sensor data TX fails we will reconnect. 
		*/
		break;
	}
}

static void appStateAwsSendSenorData(void)
{
	int rc = 0;

	MAIN_LOG_DBG("AWS send sensor data state");

	k_sem_take(&send_data_sem, K_FOREVER);

	lteInfo = lteGetStatus();

	k_mutex_lock(&sensor_data_lock, K_FOREVER);
	MAIN_LOG_INF("Sending Sensor data t:%d, h:%d, p:%d...",
		     (int)(temperatureReading * 100),
		     (int)(humidityReading * 100), pressureReading);
	rc = awsPublishSensorData(temperatureReading, humidityReading,
				  pressureReading, lteInfo->rssi);
	if (rc != 0) {
		MAIN_LOG_ERR("Could not send sensor data (%d)", rc);
		led_flash_red();
		appState = appStateAwsDisconnect;
	} else {
		MAIN_LOG_INF("Data sent");
		led_flash_green();
	}

	k_mutex_unlock(&sensor_data_lock);
}

static void appStateAwsInitShadow(void)
{
	int rc;

	MAIN_LOG_DBG("AWS init shadow state");

	/* Fill in base shadow info and publish */
	awsSetShadowAppFirmwareVersion(APP_VERSION_STRING);
	awsSetShadowKernelVersion(KERNEL_VERSION_STRING);
	awsSetShadowIMEI(lteInfo->IMEI);
	awsSetShadowRadioFirmwareVersion(lteInfo->radio_version);

	MAIN_LOG_INF("Send persistent shadow data");
	rc = awsPublishShadowPersistentData();
	if (rc != 0) {
		appState = appStateAwsDisconnect;
		goto exit;
	}
	initShadow = false;
	/* The shadow init is only sent once after the very first connect.
	*  we want to send the first sensor data ASAP after the shadow is inited
	*/
	sendSensorDataAsap = true;
	appState = appStateAwsSendSenorData;
exit:
	return;
}

static void appStateAwsConnect(void)
{
	MAIN_LOG_DBG("AWS connect state");

	if (!lteIsReady()) {
		appState = appStateWaitForLte;
		return;
	}

	while (awsConnect(lteInfo->IMEI) != 0) {
		MAIN_LOG_ERR("Could not connect to aws, retrying in %d seconds",
			     RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(RETRY_AWS_ACTION_TIMEOUT_SECONDS));
	}

	if (initShadow) {
		/* Init the shadow once, the first time we connect */
		appState = appStateAwsInitShadow;
	} else {
		/* after a connection we want to send the first sensor data ASAP */
		sendSensorDataAsap = true;
		appState = appStateAwsSendSenorData;
	}
}

static void appStateAwsDisconnect(void)
{
	MAIN_LOG_DBG("AWS disconnect state");
	stopSendDataTimer();
	awsDisconnect();
	appState = appStateAwsConnect;
}

static void appStateAwsResolveServer(void)
{
	MAIN_LOG_DBG("AWS resolve server state");

	while (awsGetServerAddr() != 0) {
		MAIN_LOG_ERR(
			"Could not get server address, retrying in %d seconds",
			RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(RETRY_AWS_ACTION_TIMEOUT_SECONDS));
	}
	resolveAwsServer = false;
	appState = appStateAwsConnect;
}

static void appStateWaitForLte(void)
{
	MAIN_LOG_DBG("Wait for LTE state");

	if (!lteIsReady()) {
		/* Wait for LTE read evt */
		k_sem_reset(&lte_ready_sem);
		k_sem_take(&lte_ready_sem, K_FOREVER);
	}

	if (resolveAwsServer) {
		appState = appStateAwsResolveServer;
	} else {
		appState = appStateAwsConnect;
	}
}

void main(void)
{
	int rc;

	/* init LEDS */
	led_init();

	/* init LTE */
	lteRegisterEventCallback(lteEvent);
	rc = lteInit();
	if (rc < 0) {
		MAIN_LOG_ERR("LTE init (%d)", rc);
		goto exit;
	}
	lteInfo = lteGetStatus();

	/* Start up BLE portion of the demo */
	oob_ble_initialise();
	oob_ble_set_callback(SensorUpdated);

	/* init AWS */
	rc = awsInit();
	if (rc != 0) {
		goto exit;
	}

	appState = appStateWaitForLte;

	while (true) {
		appState();
	}
exit:
	MAIN_LOG_ERR("Exiting main thread");
	return;
}
