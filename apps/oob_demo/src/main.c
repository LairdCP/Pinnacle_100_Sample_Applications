/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_main);

#include <stdio.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <version.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>

#include "oob_common.h"
#include "led.h"
#include "oob_ble.h"
#include "lte.h"
#include "aws.h"
#include "nv.h"
#include "ble_cellular_service.h"

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
static void appStateCommissionDevice(void);

K_TIMER_DEFINE(SendDataTimer, SendDataTimerExpired, NULL);
K_SEM_DEFINE(send_data_sem, 0, 1);
K_MUTEX_DEFINE(sensor_data_lock);
K_SEM_DEFINE(lte_ready_sem, 0, 1);
K_SEM_DEFINE(rx_cert_sem, 0, 1);

static float temperatureReading = 0;
static float humidityReading = 0;
static u32_t pressureReading = 0;
static bool initShadow = true;
static bool sendSensorDataAsap = true;
static bool resolveAwsServer = true;

static bool bUpdatedTemperature = false;
static bool bUpdatedHumidity = false;
static bool bUpdatedPressure = false;

static bool commissioned;
static bool allowCommissioning = false;
static bool appReady = false;
static bool devCertSet;
static bool devKeySet;
static u8_t devCert[AWS_DEV_CERT_MAX_LENGTH];
static u8_t devKey[AWS_DEV_KEY_MAX_LENGTH];

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
		cell_svc_notify_status(oob_ble_get_central_connection(),
				       CELL_STATUS_CONNECTED);
		break;
	case LTE_EVT_DISCONNECTED:
		/* No need to trigger a reconnect.
		*  If next sensor data TX fails we will reconnect. 
		*/
		cell_svc_notify_status(oob_ble_get_central_connection(),
				       CELL_STATUS_DISCONNECTED);

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
				  pressureReading, lteInfo->rssi,
				  lteInfo->sinr);
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
	awsSetShadowICCID(lteInfo->ICCID);
	awsSetShadowRadioFirmwareVersion(lteInfo->radio_version);
	awsSetShadowRadioSerialNumber(lteInfo->serialNumber);

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

	if (awsConnect(lteInfo->IMEI) != 0) {
		MAIN_LOG_ERR("Could not connect to aws, retrying in %d seconds",
			     RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(RETRY_AWS_ACTION_TIMEOUT_SECONDS));
		return;
	}

	nvStoreCommissioned(true);

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
	if (devCertSet && devKeySet) {
		appState = appStateAwsConnect;
	} else {
		appState = appStateCommissionDevice;
	}
}

static void appStateAwsResolveServer(void)
{
	MAIN_LOG_DBG("AWS resolve server state");

	if (!lteIsReady()) {
		appState = appStateWaitForLte;
		return;
	}

	if (awsGetServerAddr() != 0) {
		MAIN_LOG_ERR(
			"Could not get server address, retrying in %d seconds",
			RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(RETRY_AWS_ACTION_TIMEOUT_SECONDS));
		return;
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
	} else {
		cell_svc_set_status(CELL_STATUS_CONNECTED);
	}

	if (resolveAwsServer) {
		appState = appStateAwsResolveServer;
	} else {
		appState = appStateAwsConnect;
	}
}

static int setAwsCredentials(void)
{
	int rc;

	rc = nvReadDevCert(devCert, sizeof(devCert));
	if (rc <= 0) {
		MAIN_LOG_ERR("Reading device cert (%d)", rc);
		return APP_ERR_READ_CERT;
	}

	rc = nvReadDevKey(devKey, sizeof(devKey));
	if (rc <= 0) {
		MAIN_LOG_ERR("Reading device key (%d)", rc);
		return APP_ERR_READ_KEY;
	}
	devCertSet = true;
	devKeySet = true;
	rc = awsSetCredentials(devCert, devKey);
	return rc;
}

static void appStateCommissionDevice(void)
{
	MAIN_LOG_DBG("Commission device state");
	printk("\n\nWaiting to commission device\n\n");
	allowCommissioning = true;

	k_sem_take(&rx_cert_sem, K_FOREVER);
	if (setAwsCredentials() == 0) {
		appState = appStateWaitForLte;
	}
}

char *replaceWord(const char *s, const char *oldW, const char *newW, char *dest,
		  int destSize)
{
	int i, cnt = 0;
	int newWlen = strlen(newW);
	int oldWlen = strlen(oldW);

	/* Counting the number of times old word
	*  occur in the string 
	*/
	for (i = 0; s[i] != '\0'; i++) {
		if (strstr(&s[i], oldW) == &s[i]) {
			cnt++;

			/* Jumping to index after the old word. */
			i += oldWlen - 1;
		}
	}

	/* Make sure new string isnt too big */
	if ((i + cnt * (newWlen - oldWlen) + 1) > destSize) {
		return NULL;
	}

	i = 0;
	while (*s) {
		/* compare the substring with the result */
		if (strstr(s, oldW) == s) {
			strcpy(&dest[i], newW);
			i += newWlen;
			s += oldWlen;
		} else {
			dest[i++] = *s++;
		}
	}

	dest[i] = '\0';
	return dest;
}

static int setCert(enum CREDENTIAL_TYPE type, u8_t *cred)
{
	int rc;
	int certSize;
	int expSize = 0;
	char *newCred = NULL;

	if (!appReady) {
		printk("App is not ready\n");
		return APP_ERR_NOT_READY;
	}

	if (!allowCommissioning) {
		printk("Not ready for commissioning, decommission device first\n");
		return APP_ERR_COMMISSION_DISALLOWED;
	}

	certSize = strlen(cred);

	if (type == CREDENTIAL_CERT) {
		expSize = sizeof(devCert);
		newCred = devCert;
	} else if (type == CREDENTIAL_KEY) {
		expSize = sizeof(devKey);
		newCred = devKey;
	}

	if (certSize > expSize) {
		printk("Cert is too large (%d)\n", certSize);
		return APP_ERR_CRED_TOO_LARGE;
	}

	replaceWord(cred, "\\n", "\n", newCred, expSize);
	replaceWord(newCred, "\\s", " ", newCred, expSize);

	if (type == CREDENTIAL_CERT) {
		rc = nvStoreDevCert(newCred, strlen(newCred));
		if (rc >= 0) {
			printk("Stored cert:\n%s\n", newCred);
			devCertSet = true;
		} else {
			MAIN_LOG_ERR("Error storing cert (%d)", rc);
		}
	} else if (type == CREDENTIAL_KEY) {
		rc = nvStoreDevKey(newCred, strlen(newCred));
		if (rc >= 0) {
			printk("Stored key:\n%s\n", newCred);
			devKeySet = true;
		} else {
			MAIN_LOG_ERR("Error storing key (%d)", rc);
		}
	} else {
		rc = APP_ERR_UNKNOWN_CRED;
	}

	if (rc >= 0 && devCertSet && devKeySet) {
		k_sem_give(&rx_cert_sem);
	}

	return rc;
}

static int set_aws_device_cert(const struct shell *shell, size_t argc,
			       char **argv)
{
	ARG_UNUSED(argc);

	return setCert(CREDENTIAL_CERT, argv[1]);
}

static int set_aws_device_key(const struct shell *shell, size_t argc,
			      char **argv)
{
	ARG_UNUSED(argc);

	return setCert(CREDENTIAL_KEY, argv[1]);
}

static int decommission(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!appReady) {
		printk("App is not ready\n");
		return APP_ERR_NOT_READY;
	}

	nvDeleteDevCert();
	nvDeleteDevKey();
	devCertSet = false;
	devKeySet = false;

	nvStoreCommissioned(false);
	commissioned = false;
	allowCommissioning = true;
	appState = appStateAwsDisconnect;
	printk("Device is decommissioned\n");

	return 0;
}

void main(void)
{
	int rc;

	/* init LEDS */
	led_init();

	/* Init NV storage */
	rc = nvInit();
	if (rc < 0) {
		MAIN_LOG_ERR("NV init (%d)", rc);
		goto exit;
	}

	nvReadCommissioned(&commissioned);

	/* init LTE */
	lteRegisterEventCallback(lteEvent);
	rc = lteInit();
	if (rc < 0) {
		MAIN_LOG_ERR("LTE init (%d)", rc);
		goto exit;
	}
	lteInfo = lteGetStatus();

	/* Start up BLE portion of the demo */
	cell_svc_set_imei(lteInfo->IMEI);
	cell_svc_set_fw_ver(lteInfo->radio_version);
	oob_ble_initialise();
	oob_ble_set_callback(SensorUpdated);

	/* init AWS */
	rc = awsInit();
	if (rc != 0) {
		goto exit;
	}

	appReady = true;
	printk("\n!!!!!!!! App is ready! !!!!!!!!\n");

	if (commissioned && setAwsCredentials() == 0) {
		appState = appStateWaitForLte;
	} else {
		appState = appStateCommissionDevice;
	}

	while (true) {
		appState();
	}
exit:
	MAIN_LOG_ERR("Exiting main thread");
	return;
}

SHELL_STATIC_SUBCMD_SET_CREATE(oob_cmds,
			       SHELL_CMD_ARG(set_cert, NULL, "Set device cert",
					     set_aws_device_cert, 2, 0),
			       SHELL_CMD_ARG(set_key, NULL, "Set device key",
					     set_aws_device_key, 2, 0),
			       SHELL_CMD(reset, NULL,
					 "Factory reset (decommission) device",
					 decommission),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(oob, &oob_cmds, "OOB Demo commands", NULL);
