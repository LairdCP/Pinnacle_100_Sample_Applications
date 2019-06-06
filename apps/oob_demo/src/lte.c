/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_lte);

#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <net/socket.h>
#include <net/mqtt.h>

#include <string.h>
#include <errno.h>

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

static struct k_sem iface_ready;
static bool reconnect = false;

static void iface_ready_evt_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	LTE_LOG_DBG("iface IPv4 addr added!");
	k_sem_give(&iface_ready);
}

static void iface_down_evt_handler(struct net_mgmt_event_callback *cb,
				   u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IF_DOWN) {
		return;
	}

	LTE_LOG_WRN("iface is down");
	reconnect = true;
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

void lte_thread(void *param1, void *param2, void *param3)
{
	int rc;
	struct net_if *iface;
	struct net_if_config *cfg;

	k_sem_init(&iface_ready, 0, 1);

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
		k_sem_take(&iface_ready, K_FOREVER);
	}

	LTE_LOG_INF("LTE ready");

	rc = awsInit();
	if (rc != 0) {
		goto exit;
	}

	rc = awsGetServerAddr();
	if (rc != 0) {
		goto exit;
	}

	rc = awsConnect();
	if (rc != 0) {
		goto exit;
	}

	reconnect = false;
	while (true) {
		if (reconnect) {
			reconnect = false;
			/* cleanup */
			awsDisconnect();
			LTE_LOG_INF("Waiting for network IP addr...");
			k_sem_take(&iface_ready, K_FOREVER);
			/* re-connect */
			rc = awsConnect();
			if (rc != 0) {
				goto exit;
			}
		}
		LTE_LOG_INF("Sending data...");
		rc = awsSendData(0);
		if (rc != 0) {
			/* re-connect */
			awsDisconnect();
			rc = awsConnect();
			if (rc != 0) {
				goto exit;
			}
		} else {
			LTE_LOG_INF("Data sent");
		}
		k_sleep(K_MINUTES(1));
	}
exit:
	return;
}

K_THREAD_DEFINE(oob_lte, LTE_STACK_SIZE, lte_thread, NULL, NULL, NULL,
		LTE_THREAD_PRIORITY, 0, K_NO_WAIT);
