/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power/power.h>
#include <string.h>
#include <net/socket.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(edrx_tcp);

#include "config.h"
#include "led.h"
#include "lte.h"

K_SEM_DEFINE(lte_event_sem, 0, 1);

static enum lte_event lte_status = LTE_EVT_DISCONNECTED;
static char response[128];

static void lteEvent(enum lte_event event)
{
	lte_status = event;
	k_sem_give(&lte_event_sem);
}

static int tcp_connection()
{
	int ret, len, sock = 0;
	struct sockaddr_in dest;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("creating socket failed");
		return 1;
	}

	dest.sin_family = AF_INET;
	ret = net_addr_pton(AF_INET, SERVER_IP_ADDR, &dest.sin_addr);
	if (ret < 0) {
		LOG_ERR("setting dest IP addr failed (%d)", ret);
		return ret;
	}
	dest.sin_port = htons(SERVER_PORT);

	ret = connect(sock, (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		LOG_ERR("connect socket failed (%d)", ret);
		return ret;
	}

	while (true) {
		LOG_INF("Receiving data...");
		len = recv(sock, response, sizeof(response) - 1, 0);

		if (len == -EAGAIN) {
			LOG_ERR("RX timeout (%d)", len);
			break;
		} else if (len == -EWOULDBLOCK) {
			LOG_ERR("RX EWOULDBLOCK (%d)", len);
			break;
		} else if (len < 0) {
			LOG_ERR("RX unknown err (%d)", len);
			break;
		} else if (len == 0) {
			/* socket shutdown properly */
			LOG_INF("Socket closed");
			break;
		}

		response[len] = 0;
		LOG_INF("RX: %s", response);
	}

	close(sock);
	return ret;
}

/* Application main Thread */
void main(void)
{
	LOG_INF("*** eDRX TCP Server Reply test on %s ***", CONFIG_BOARD);

	led_init();

	/* init LTE */
	lteRegisterEventCallback(lteEvent);
	int rc = lteInit();
	if (rc < 0) {
		LOG_INF("LTE init (%d)", rc);
		goto exit;
	}

	if (!lteIsReady()) {
		while (lte_status != LTE_EVT_READY) {
			/* Wait for LTE read evt */
			k_sem_reset(&lte_event_sem);
			LOG_INF("Waiting for LTE to be ready...");
			k_sem_take(&lte_event_sem, K_FOREVER);
		}
	}
	LOG_INF("LTE ready!");

	while (1) {
		tcp_connection();
		LOG_INF("App sleeping for %d seconds", SLEEP_TIME_SECONDS);
		k_sleep(K_SECONDS(SLEEP_TIME_SECONDS));
	}
exit:
	LOG_ERR("Exiting main()");
	return;
}