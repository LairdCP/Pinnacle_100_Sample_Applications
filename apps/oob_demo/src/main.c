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
#include "ble_aws_service.h"

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
		cell_svc_set_status(oob_ble_get_central_connection(),
				    CELL_STATUS_CONNECTED);
		break;
	case LTE_EVT_DISCONNECTED:
		/* No need to trigger a reconnect.
		*  If next sensor data TX fails we will reconnect. 
		*/
		cell_svc_set_status(oob_ble_get_central_connection(),
				    CELL_STATUS_DISCONNECTED);

		break;
	}
}

static void appStateAwsSendSenorData(void)
{
	int rc = 0;

	MAIN_LOG_DBG("AWS send sensor data state");

	k_sem_take(&send_data_sem, K_FOREVER);

	if (!commissioned) {
		/* we were decommissioned, disconnect */
		appState = appStateAwsDisconnect;
	}

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

	if (awsConnect() != 0) {
		MAIN_LOG_ERR("Could not connect to aws, retrying in %d seconds",
			     RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		aws_svc_set_status(oob_ble_get_central_connection(),
				   AWS_STATUS_CONNECTION_ERR);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(RETRY_AWS_ACTION_TIMEOUT_SECONDS));
		return;
	}

	nvStoreCommissioned(true);
	commissioned = true;
	allowCommissioning = false;

	aws_svc_set_status(oob_ble_get_central_connection(),
			   AWS_STATUS_CONNECTED);

	if (initShadow) {
		/* Init the shadow once, the first time we connect */
		appState = appStateAwsInitShadow;
	} else {
		/* after a connection we want to send the first sensor data ASAP */
		sendSensorDataAsap = true;
		appState = appStateAwsSendSenorData;
	}
}

static bool areCertsSet(void)
{
    return (devCertSet && devKeySet);
}

static void appStateAwsDisconnect(void)
{
	MAIN_LOG_DBG("AWS disconnect state");
	aws_svc_set_status(oob_ble_get_central_connection(),
			   AWS_STATUS_DISCONNECTED);
	stopSendDataTimer();
	awsDisconnect();
	if (areCertsSet()) {
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

	aws_svc_set_status(oob_ble_get_central_connection(),
			   AWS_STATUS_DISCONNECTED);

	if (!lteIsReady()) {
		/* Wait for LTE read evt */
		k_sem_reset(&lte_ready_sem);
		k_sem_take(&lte_ready_sem, K_FOREVER);
	} else {
		cell_svc_set_status(oob_ble_get_central_connection(),
				    CELL_STATUS_CONNECTED);
	}

	if (resolveAwsServer && areCertsSet()) {
		appState = appStateAwsResolveServer;
	} else if (areCertsSet()) {
		appState = appStateAwsConnect;
	} else {
		appState = appStateCommissionDevice;
	}
}

static int setAwsCredentials(void)
{
	int rc;

	if (!aws_svc_client_cert_is_stored()) {
		return APP_ERR_READ_CERT;
	}

	if (!aws_svc_client_key_is_stored()) {
		return APP_ERR_READ_KEY;
	}
	devCertSet = true;
	devKeySet = true;
	rc = awsSetCredentials(aws_svc_get_client_cert(),
			       aws_svc_get_client_key());
	return rc;
}

static void appStateCommissionDevice(void)
{
	MAIN_LOG_DBG("Commission device state");
	printk("\n\nWaiting to commission device\n\n");
	aws_svc_set_status(oob_ble_get_central_connection(),
			   AWS_STATUS_NOT_PROVISIONED);
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

static void decommission(void)
{
	nvStoreCommissioned(false);
	devCertSet = false;
	devKeySet = false;
	commissioned = false;
	allowCommissioning = true;
	appState = appStateAwsDisconnect;
	printk("Device is decommissioned\n");
	/* trigger send data in case we are waiting in that state */
	k_sem_give(&send_data_sem);
}

static void awsSvcEvent(enum aws_svc_event event)
{
	switch (event) {
	case AWS_SVC_EVENT_SETTINGS_SAVED:
		devCertSet = true;
		devKeySet = true;
		k_sem_give(&rx_cert_sem);
		break;
	case AWS_SVC_EVENT_SETTINGS_CLEARED:
		decommission();
		break;
	}
}

#ifdef CONFIG_SHELL
static int shellSetCert(enum CREDENTIAL_TYPE type, u8_t *cred)
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
		expSize = AWS_CLIENT_CERT_MAX_LENGTH;
		newCred = (char *)aws_svc_get_client_cert();
	} else if (type == CREDENTIAL_KEY) {
		expSize = AWS_CLIENT_KEY_MAX_LENGTH;
		newCred = (char *)aws_svc_get_client_key();
	} else {
		return APP_ERR_UNKNOWN_CRED;
	}

	if (certSize > expSize) {
		printk("Cert is too large (%d)\n", certSize);
		return APP_ERR_CRED_TOO_LARGE;
	}

	replaceWord(cred, "\\n", "\n", newCred, expSize);
	replaceWord(newCred, "\\s", " ", newCred, expSize);

	rc = aws_svc_save_clear_settings(true);
	if (rc < 0) {
		MAIN_LOG_ERR("Error storing credential (%d)", rc);
	} else if (type == CREDENTIAL_CERT) {
		printk("Stored cert:\n%s\n", newCred);
		devCertSet = true;

	} else if (type == CREDENTIAL_KEY) {
		printk("Stored key:\n%s\n", newCred);
		devKeySet = true;
	}

	if (rc >= 0 && areCertsSet()) {
		k_sem_give(&rx_cert_sem);
	}

	return rc;
}

static int shell_set_aws_device_cert(const struct shell *shell, size_t argc,
				     char **argv)
{
	ARG_UNUSED(argc);

	return shellSetCert(CREDENTIAL_CERT, argv[1]);
}

static int shell_set_aws_device_key(const struct shell *shell, size_t argc,
				    char **argv)
{
	ARG_UNUSED(argc);

	return shellSetCert(CREDENTIAL_KEY, argv[1]);
}

static int shell_decommission(const struct shell *shell, size_t argc,
			      char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!appReady) {
		printk("App is not ready\n");
		return APP_ERR_NOT_READY;
	}

	aws_svc_save_clear_settings(false);
	decommission();

	return 0;
}
#endif /* CONFIG_SHELL */

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

	/* init AWS */
	rc = awsInit();
	if (rc != 0) {
		goto exit;
	}

	/* Start up BLE portion of the demo */
	cell_svc_set_imei(lteInfo->IMEI);
	cell_svc_set_fw_ver(lteInfo->radio_version);
	rc = aws_svc_init(lteInfo->IMEI);
	if (rc != 0) {
		goto exit;
	}
	aws_svc_set_event_callback(awsSvcEvent);
	if (commissioned) {
		aws_svc_set_status(NULL, AWS_STATUS_DISCONNECTED);

	} else {
		aws_svc_set_status(NULL, AWS_STATUS_NOT_PROVISIONED);
	}
	oob_ble_initialise();
	oob_ble_set_callback(SensorUpdated);

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

#ifdef CONFIG_SHELL
SHELL_STATIC_SUBCMD_SET_CREATE(oob_cmds,
			       SHELL_CMD_ARG(set_cert, NULL, "Set device cert",
					     shell_set_aws_device_cert, 2, 0),
			       SHELL_CMD_ARG(set_key, NULL, "Set device key",
					     shell_set_aws_device_key, 2, 0),
			       SHELL_CMD(reset, NULL,
					 "Factory reset (decommission) device",
					 shell_decommission),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(oob, &oob_cmds, "OOB Demo commands", NULL);
#endif /* CONFIG_SHELL */
