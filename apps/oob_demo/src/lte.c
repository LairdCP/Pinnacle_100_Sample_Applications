/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_lte);

#include <zephyr.h>
#include <version.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <drivers/modem/modem_receiver.h>

#include <net/socket.h>
#include <net/mqtt.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "oob_common.h"
#include "lte.h"
#include "aws.h"

#define LTE_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define LTE_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define LTE_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define LTE_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

struct mgmt_events {
	u32_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
};

K_SEM_DEFINE(iface_ready_sem, 0, 1);
K_SEM_DEFINE(reconnect_sem, 0, 1);

static struct net_if *iface;
static struct net_if_config *cfg;
struct mdm_receiver_context *mdm_rcvr;
static bool connectedToAws = false;

static void iface_ready_evt_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	LTE_LOG_DBG("iface IPv4 addr added!");
	k_sem_give(&iface_ready_sem);
}

static void iface_down_evt_handler(struct net_mgmt_event_callback *cb,
				   u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_DOWN) {
		return;
	}

	LTE_LOG_WRN("iface is down");
	connectedToAws = false;
	k_sem_give(&reconnect_sem);
}

static struct mgmt_events iface_events[] = {
	{ .event = NET_EVENT_IPV4_ADDR_ADD,
	  .handler = iface_ready_evt_handler },
	{ .event = NET_EVENT_IF_DOWN, .handler = iface_down_evt_handler },
	{ 0 }
};

static void setup_iface_events(void)
{
	int i;

	for (i = 0; iface_events[i].event; i++) {
		net_mgmt_init_event_callback(&iface_events[i].cb,
					     iface_events[i].handler,
					     iface_events[i].event);

		net_mgmt_add_event_callback(&iface_events[i].cb);
	}
}

static bool isLteReady()
{
	return net_if_is_up(iface) && cfg->ip.ipv4;
}

/**
 * @brief This function blocks until a the AWS server address is obtained
 * 
 */
static void getAwsServerAddress()
{
	while (awsGetServerAddr() != 0) {
		LTE_LOG_ERR(
			"Could not get server address, retrying in %d seconds",
			LTE_RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(LTE_RETRY_AWS_ACTION_TIMEOUT_SECONDS));
	}
}

/**
 * @brief This function blocks until a connection to AWS is successful
 * 
 */
static void connectAws()
{
	while (awsConnect() != 0) {
		LTE_LOG_ERR("Could not connect to aws, retrying in %d seconds",
			    LTE_RETRY_AWS_ACTION_TIMEOUT_SECONDS);
		/* wait some time before trying again */
		k_sleep(K_SECONDS(LTE_RETRY_AWS_ACTION_TIMEOUT_SECONDS));
	}
	k_sem_reset(&reconnect_sem);
	connectedToAws = true;
}

int lteSendSensorData(float temperature, float humidity, int pressure)
{
	int rc;

	if (!connectedToAws) {
		return LTE_ERROR_NOT_CONNECTED;
	}

	char msg[strlen(SHADOW_REPORTED_START) + strlen(SHADOW_TEMPERATURE) +
		 10 + strlen(SHADOW_HUMIDITY) + 10 + strlen(SHADOW_PRESSURE) +
		 10 + strlen(SHADOW_RADIO_RSSI) + 10 + strlen(SHADOW_END)];

	snprintf(msg, sizeof(msg), "%s%s%.2f,%s%.2f,%s%d,%s%d%s",
		 SHADOW_REPORTED_START, SHADOW_TEMPERATURE, temperature,
		 SHADOW_HUMIDITY, humidity, SHADOW_PRESSURE, pressure,
		 SHADOW_RADIO_RSSI, mdm_rcvr->data_rssi, SHADOW_END);
	rc = awsSendData(msg);
	if (rc != 0) {
		/* error sending data, lets trigger a re-connect to AWS */
		connectedToAws = false;
		k_sem_give(&reconnect_sem);
	}
	return rc;
}

void lte_thread(void *param1, void *param2, void *param3)
{
	int rc;

	setup_iface_events();

	/* wait for network interface to be ready */
	iface = net_if_get_default();
	if (!iface) {
		LTE_LOG_ERR("Could not get iface");
		return;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		LTE_LOG_ERR("Could not get iface config");
		return;
	}

	/* check if the iface has an IP */
	if (!cfg->ip.ipv4) {
		/* if no IP yet, wait for one */
		LTE_LOG_INF("Waiting for network IP addr...");
		k_sem_take(&iface_ready_sem, K_FOREVER);
	}

	LTE_LOG_INF("LTE ready");

	rc = awsInit();
	if (rc != 0) {
		goto exit;
	}

	getAwsServerAddress();
	connectAws();

	/* Fill in base shadow info and publish */
	/* Get the modem receive context */
	mdm_rcvr = mdm_receiver_context_from_id(0);
	if (mdm_rcvr == NULL) {
		LTE_LOG_ERR("Invalid modem receiver");
		goto exit;
	}

	awsSetShadowAppFirmwareVersion(APP_VERSION_STRING);
	awsSetShadowKernelVersion(KERNEL_VERSION_STRING);
	awsSetShadowIMEI(mdm_rcvr->data_imei);
	awsSetShadowRadioFirmwareVersion(mdm_rcvr->data_revision);

	LTE_LOG_INF("Send persistent shadow data");
	rc = awsPublishShadowPersistentData();
	if (rc != 0) {
		goto exit;
	}

	while (true) {
		k_sem_take(&reconnect_sem, K_FOREVER);
		LTE_LOG_INF("Reconnecting to AWS...");
		awsDisconnect();
		k_sleep(K_SECONDS(1));
		if (!isLteReady()) {
			LTE_LOG_INF("Waiting for network IP addr...");
			k_sem_take(&iface_ready_sem, K_FOREVER);
		}
		LTE_LOG_INF("Connecting to AWS...");
		connectAws();
		LTE_LOG_INF("Connected to AWS");
	}
exit:
	LTE_LOG_ERR("Quitting LTE thread!");
	return;
}

K_THREAD_DEFINE(oob_lte, LTE_STACK_SIZE, lte_thread, NULL, NULL, NULL,
		LTE_THREAD_PRIORITY, 0, K_NO_WAIT);
