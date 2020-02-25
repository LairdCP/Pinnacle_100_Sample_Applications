/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(coap_dtls, LOG_LEVEL_DBG);

#include "laird_coap.h"
#include "lte.h"
#include "certificate.h"

#define MAIN_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define MAIN_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define MAIN_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define MAIN_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define SUCCESS_MSG "Success"
#define PUT_OBS_VAL "Updated the observable endpoint!"
/* TODO: hopefully we can remove this delay in the future */
#define DELAY_TIME_SECONDS 5

K_SEM_DEFINE(lte_ready_sem, 0, 1);

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

void main(void)
{
	int r;
	struct coap_packet reply;
	const u8_t *replyPayload = NULL;
	u16_t replyPayloadLength;

	lteRegisterEventCallback(lteEvent);
	r = lteInit();
	if (r < 0) {
		MAIN_LOG_ERR("LTE init (%d)", r);
		goto err;
	}

	if (!lteIsReady()) {
		MAIN_LOG_INF("Waiting for network");
		/* Wait for LTE read evt */
		k_sem_reset(&lte_ready_sem);
		k_sem_take(&lte_ready_sem, K_FOREVER);
	}

	r = laird_coap_init(ca_certificate, sizeof(ca_certificate),
			    dev_certificate, sizeof(dev_certificate), dev_key,
			    sizeof(dev_key));
	if (r < 0) {
		goto err;
	}

	r = laird_coap_get_server_addr();
	if (r < 0) {
		goto err;
	}

	MAIN_LOG_DBG("Start CoAP DTLS");
	r = laird_coap_start_client();
	if (r < 0) {
		goto err;
	}

	MAIN_LOG_INF("Test GET /ping");
	r = laird_coap_send_request_wait_for_reply(COAP_METHOD_GET,
						   COAP_PATH_PING, NULL, 0,
						   &reply, &replyPayload,
						   &replyPayloadLength);
	if (r < 0) {
		goto err;
	}
	/* compare response with expected response */
	r = strcmp(COAP_RESPONSE_PING, replyPayload);
	if (r != 0) {
		MAIN_LOG_ERR("%s response [%s] does not match expected [%s]",
			     COAP_PATH_PING, replyPayload, COAP_RESPONSE_PING);
	} else {
		MAIN_LOG_INF(SUCCESS_MSG);
	}
	/* free coap packet data now that we are done with it*/
	laird_coap_free_pkt_data(&reply);

	MAIN_LOG_INF("Test GET /observable");
	r = laird_coap_send_request_wait_for_reply(COAP_METHOD_GET,
						   COAP_PATH_OBSERVABLE, NULL,
						   0, &reply, &replyPayload,
						   &replyPayloadLength);
	if (r < 0) {
		goto err;
	}
	/* compare response with expected response */
	r = strcmp(COAP_RESPONSE_DEFAULT_OBSERVABLE, replyPayload);
	if (r != 0) {
		MAIN_LOG_ERR("%s response [%s] does not match expected [%s]",
			     COAP_PATH_OBSERVABLE, replyPayload,
			     COAP_RESPONSE_DEFAULT_OBSERVABLE);
	} else {
		MAIN_LOG_INF(SUCCESS_MSG);
	}
	/* free coap packet data now that we are done with it*/
	laird_coap_free_pkt_data(&reply);

	MAIN_LOG_INF("Test PUT /observable");
	r = laird_coap_send_request(COAP_METHOD_PUT, COAP_PATH_OBSERVABLE,
				    PUT_OBS_VAL, strlen(PUT_OBS_VAL));
	if (r < 0) {
		goto err;
	}

	/* wait some time for server update to take effect */
	k_sleep(K_SECONDS(DELAY_TIME_SECONDS));

	MAIN_LOG_INF("Test MODIFIED GET /observable");
	r = laird_coap_send_request_wait_for_reply(COAP_METHOD_GET,
						   COAP_PATH_OBSERVABLE, NULL,
						   0, &reply, &replyPayload,
						   &replyPayloadLength);
	if (r < 0) {
		goto err;
	}
	/* compare response with expected response */
	r = strcmp(PUT_OBS_VAL, replyPayload);
	if (r != 0) {
		MAIN_LOG_ERR("%s response [%s] does not match expected [%s]",
			     COAP_PATH_OBSERVABLE, replyPayload, PUT_OBS_VAL);
	} else {
		MAIN_LOG_INF(SUCCESS_MSG);
	}
	/* free coap packet data now that we are done with it*/
	laird_coap_free_pkt_data(&reply);

	MAIN_LOG_INF("Test DELETE /observable");
	r = laird_coap_send_request(COAP_METHOD_DELETE, COAP_PATH_OBSERVABLE,
				    NULL, 0);
	if (r < 0) {
		goto err;
	}

	/* wait some time for server update to take effect */
	k_sleep(K_SECONDS(DELAY_TIME_SECONDS));

	MAIN_LOG_INF("Test GET /observable is set back to default");
	r = laird_coap_send_request_wait_for_reply(COAP_METHOD_GET,
						   COAP_PATH_OBSERVABLE, NULL,
						   0, &reply, &replyPayload,
						   &replyPayloadLength);
	if (r < 0) {
		goto err;
	}
	/* compare response with expected response */
	r = strcmp(COAP_RESPONSE_DEFAULT_OBSERVABLE, replyPayload);
	if (r != 0) {
		MAIN_LOG_ERR("%s response [%s] does not match expected [%s]",
			     COAP_PATH_OBSERVABLE, replyPayload,
			     COAP_RESPONSE_DEFAULT_OBSERVABLE);
	} else {
		MAIN_LOG_INF(SUCCESS_MSG);
	}
	/* free coap packet data now that we are done with it*/
	laird_coap_free_pkt_data(&reply);

	MAIN_LOG_DBG("Done");
	goto done;
err:
	MAIN_LOG_ERR("quit");
done:
	/* Close the socket */
	(void)laird_coap_close_client();
}
