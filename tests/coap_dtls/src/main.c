/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(coap_dtls, LOG_LEVEL_DBG);

#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>

#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>
#include <mbedtls/ssl.h>
#include <net/tls_credentials.h>

#include "net_private.h"
#include "lte.h"

#define SERVER_IP_ADDR "18.232.90.30"
#define SERVER_PORT 5684
#define MAX_COAP_MSG_LEN 256

#define CA_CERTIFICATE_TAG 1

static const unsigned char ca_certificate[] = {
#include "root_server_cert.der.inc"
};

static sec_tag_t sock_sec_tags[] = { CA_CERTIFICATE_TAG };

#define MAIN_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define MAIN_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define MAIN_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define MAIN_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

K_SEM_DEFINE(lte_ready_sem, 0, 1);

/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds;

/* CoAP Options */
static const char *const test_path[] = { "test", NULL };

static const char *const large_path[] = { "large", NULL };

static const char *const obs_path[] = { "obs", NULL };

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

static struct coap_block_context blk_ctx;

static void wait(void)
{
	if (poll(fds, nfds, K_FOREVER) < 0) {
		LOG_ERR("Error in poll:%d", errno);
	}
}

static void prepare_fds(void)
{
	fds[nfds].fd = sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

static int tls_init()
{
	int err = 0;

	err = tls_credential_add(CA_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE, ca_certificate,
				 sizeof(ca_certificate));
	if (err < 0) {
		MAIN_LOG_ERR("Failed to register public certificate: %d", err);
	}

	return err;
}

static int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_in dest;

	ret = tls_init();
	if (ret < 0) {
		MAIN_LOG_ERR("TLS init (%d)", ret);
		return ret;
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);
	if (sock < 0) {
		MAIN_LOG_ERR("creating socket failed");
		return 1;
	}

	ret = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sock_sec_tags,
			 sizeof(sock_sec_tags));
	if (ret < 0) {
		MAIN_LOG_ERR("Socket options (%d)", ret);
		return ret;
	}

	ret = setsockopt(sock, SOL_TLS, TLS_HOSTNAME, NULL, 0);
	if (ret < 0) {
		MAIN_LOG_ERR("Socket host name (%d)", ret);
		return ret;
	}

	dest.sin_family = AF_INET;
	dest.sin_port = htons(SERVER_PORT);
	ret = net_addr_pton(AF_INET, SERVER_IP_ADDR, &dest.sin_addr);
	if (ret < 0) {
		MAIN_LOG_ERR("setting dest IP addr failed (%d)", ret);
		return ret;
	}

	ret = connect(sock, (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		MAIN_LOG_ERR("connect socket failed (%d)", ret);
		return ret;
	}

	prepare_fds();

	return 0;
}

static int process_simple_coap_reply(void)
{
	struct coap_packet reply;
	u8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

end:
	k_free(data);

	return ret;
}

static int send_simple_coap_request(u8_t method)
{
	u8_t payload[] = "payload";
	struct coap_packet request;
	const char *const *p;
	u8_t *data;
	int r;

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_CON,
			     8, coap_next_token(), method, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = test_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	switch (method) {
	case COAP_METHOD_GET:
	case COAP_METHOD_DELETE:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			LOG_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, (u8_t *)payload,
					       sizeof(payload) - 1);
		if (r < 0) {
			LOG_ERR("Not able to append payload");
			goto end;
		}

		break;
	default:
		r = -EINVAL;
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return 0;
}

static int send_simple_coap_msgs_and_wait_for_reply(void)
{
	u8_t test_type = 0U;
	int r;

	while (1) {
		switch (test_type) {
		case 0:
			/* Test CoAP GET method */
			printk("\nCoAP client GET\n");
			r = send_simple_coap_request(COAP_METHOD_GET);
			if (r < 0) {
				return r;
			}

			break;
		case 1:
			/* Test CoAP PUT method */
			printk("\nCoAP client PUT\n");
			r = send_simple_coap_request(COAP_METHOD_PUT);
			if (r < 0) {
				return r;
			}

			break;
		case 2:
			/* Test CoAP POST method*/
			printk("\nCoAP client POST\n");
			r = send_simple_coap_request(COAP_METHOD_POST);
			if (r < 0) {
				return r;
			}

			break;
		case 3:
			/* Test CoAP DELETE method*/
			printk("\nCoAP client DELETE\n");
			r = send_simple_coap_request(COAP_METHOD_DELETE);
			if (r < 0) {
				return r;
			}

			break;
		default:
			return 0;
		}

		r = process_simple_coap_reply();
		if (r < 0) {
			return r;
		}

		test_type++;
	}

	return 0;
}

static int process_large_coap_reply(void)
{
	struct coap_packet reply;
	u8_t *data;
	bool last_block;
	int rcvd;
	int ret;

	wait();

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	ret = coap_update_from_block(&reply, &blk_ctx);
	if (ret < 0) {
		goto end;
	}

	last_block = coap_next_block(&reply, &blk_ctx);
	if (!last_block) {
		ret = 1;
		goto end;
	}

	ret = 0;

end:
	k_free(data);

	return ret;
}

static int send_large_coap_request(void)
{
	struct coap_packet request;
	const char *const *p;
	u8_t *data;
	int r;

	if (blk_ctx.total_size == 0) {
		coap_block_transfer_init(&blk_ctx, COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_CON,
			     8, coap_next_token(), COAP_METHOD_GET,
			     coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = large_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	r = coap_append_block2_option(&request, &blk_ctx);
	if (r < 0) {
		LOG_ERR("Unable to add block2 option.");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int get_large_coap_msgs(void)
{
	int r;

	while (1) {
		/* Test CoAP Large GET method */
		printk("\nCoAP client Large GET (block %zd)\n",
		       blk_ctx.current / 64 /*COAP_BLOCK_64*/);
		r = send_large_coap_request();
		if (r < 0) {
			return r;
		}

		r = process_large_coap_reply();
		if (r < 0) {
			return r;
		}

		/* Received last block */
		if (r == 1) {
			memset(&blk_ctx, 0, sizeof(blk_ctx));
			return 0;
		}
	}

	return 0;
}

static int send_obs_reply_ack(u16_t id, u8_t *token, u8_t tkl)
{
	struct coap_packet request;
	u8_t *data;
	int r;

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_ACK,
			     tkl, token, 0, id);
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);
end:
	k_free(data);

	return r;
}

static int process_obs_coap_reply(void)
{
	struct coap_packet reply;
	u16_t id;
	u8_t token[8];
	u8_t *data;
	u8_t type;
	u8_t tkl;
	int rcvd;
	int ret;

	wait();

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	tkl = coap_header_get_token(&reply, (u8_t *)token);
	id = coap_header_get_id(&reply);

	type = coap_header_get_type(&reply);
	if (type == COAP_TYPE_ACK) {
		ret = 0;
	} else if (type == COAP_TYPE_CON) {
		ret = send_obs_reply_ack(id, token, tkl);
	}
end:
	k_free(data);

	return ret;
}

static int send_obs_coap_request(void)
{
	struct coap_packet request;
	const char *const *p;
	u8_t *data;
	int r;

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_CON,
			     8, coap_next_token(), COAP_METHOD_GET,
			     coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int send_obs_reset_coap_request(void)
{
	struct coap_packet request;
	const char *const *p;
	u8_t *data;
	int r;

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1,
			     COAP_TYPE_RESET, 8, coap_next_token(), 0,
			     coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int register_observer(void)
{
	u8_t counter = 0U;
	int r;

	while (1) {
		/* Test CoAP OBS GET method */
		if (!counter) {
			printk("\nCoAP client OBS GET\n");
			r = send_obs_coap_request();
			if (r < 0) {
				return r;
			}
		} else {
			printk("\nCoAP OBS Notification\n");
		}

		r = process_obs_coap_reply();
		if (r < 0) {
			return r;
		}

		counter++;

		/* Unregister */
		if (counter == 5U) {
			/* TODO: Functionality can be verified by waiting for
			 * some time and make sure client shouldn't receive
			 * any notifications. If client still receives
			 * notifications means, Observer is not removed.
			 */
			return send_obs_reset_coap_request();
		}
	}

	return 0;
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

void main(void)
{
	int r;

	lteRegisterEventCallback(lteEvent);
	r = lteInit();
	if (r < 0) {
		MAIN_LOG_ERR("LTE init (%d)", r);
		goto quit;
	}

	if (!lteIsReady()) {
		MAIN_LOG_INF("Waiting for network");
		/* Wait for LTE read evt */
		k_sem_reset(&lte_ready_sem);
		k_sem_take(&lte_ready_sem, K_FOREVER);
	}

	MAIN_LOG_DBG("Start CoAP DTLS");
	r = start_coap_client();
	if (r < 0) {
		goto quit;
	}

	/* GET, PUT, POST, DELETE */
	r = send_simple_coap_msgs_and_wait_for_reply();
	if (r < 0) {
		goto quit;
	}

	/* Block-wise transfer */
	// Un-comment if you want to try getting lots of data
	// r = get_large_coap_msgs();
	// if (r < 0) {
	// 	goto quit;
	// }

	/* Register observer, get notifications and unregister */
	r = register_observer();
	if (r < 0) {
		goto quit;
	}

	/* Close the socket */
	(void)close(sock);

	MAIN_LOG_DBG("Done");

	return;

quit:
	(void)close(sock);

	MAIN_LOG_ERR("quit");
}
