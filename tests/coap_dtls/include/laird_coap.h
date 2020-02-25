/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LAIRD_COAP_H__
#define __LAIRD_COAP_H__

#include <net/coap.h>

#define MAX_COAP_MSG_LEN 256

#define COAP_PATH_PING "ping"
#define COAP_PATH_OBSERVABLE "observable"

#define COAP_RESPONSE_PING "pong"
#define COAP_RESPONSE_DEFAULT_OBSERVABLE "i am observable, change me\n"

int laird_coap_init(const u8_t *rootCa, size_t rootCaLen, const u8_t *cert,
		    size_t certLen, const u8_t *key, size_t keyLen);

int laird_coap_get_server_addr();

int laird_coap_start_client();

int laird_coap_send_request(enum coap_method method, const char *path,
			    u8_t *payload, u16_t payloadSize);

int laird_coap_send_request_wait_for_reply(enum coap_method method,
					   const char *path, u8_t *payload,
					   u16_t payloadSize,
					   struct coap_packet *reply,
					   const u8_t **replyPayload,
					   u16_t *replyPayloadLen);

void laird_coap_free_pkt_data(struct coap_packet *reply);

int laird_coap_close_client();

#endif /* __LAIRD_COAP_H__ */