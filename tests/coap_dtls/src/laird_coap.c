/* laird_coap.c - CoAP library
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(laird_coap);

#include <errno.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <mbedtls/ssl.h>
#include <net/tls_credentials.h>
#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>

#include "config.h"
#include "laird_coap.h"

#define COAP_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define COAP_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define COAP_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define COAP_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

#define CA_TAG 1
#define DEVICE_CERT_TAG 2

/* CoAP client socket fd */
static int sock;

static struct pollfd fds[1];
static int nfds;

#ifdef CONFIG_DNS_RESOLVER
static struct addrinfo server_addr;
static struct addrinfo *saddr;
#endif

static const sec_tag_t sock_sec_tags[] = { CA_TAG, DEVICE_CERT_TAG };

static void wait(void)
{
	if (poll(fds, nfds, K_FOREVER) < 0) {
		COAP_LOG_ERR("Error in poll:%d", errno);
	}
}

static void prepare_fds()
{
	if (nfds >= 1) {
		nfds = 0;
	}
	fds[nfds].fd = sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

static void clear_fds(void)
{
	fds[0].fd = 0;
	nfds = 0;
}

static int tls_init(const u8_t *rootCa, size_t rootCaLen, const u8_t *cert,
		    size_t certLen, const u8_t *key, size_t keyLen)
{
	int err = -EINVAL;

	tls_credential_delete(CA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE);
	err = tls_credential_add(CA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, rootCa,
				 rootCaLen);
	if (err < 0) {
		COAP_LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	tls_credential_delete(DEVICE_CERT_TAG,
			      TLS_CREDENTIAL_SERVER_CERTIFICATE);
	err = tls_credential_add(DEVICE_CERT_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE, cert,
				 certLen);
	if (err < 0) {
		COAP_LOG_ERR("Failed to register device certificate: %d", err);
		return err;
	}

	tls_credential_delete(DEVICE_CERT_TAG, TLS_CREDENTIAL_PRIVATE_KEY);
	err = tls_credential_add(DEVICE_CERT_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
				 key, keyLen);
	if (err < 0) {
		COAP_LOG_ERR("Failed to register device key: %d", err);
		return err;
	}

	return err;
}

int laird_coap_init(const u8_t *rootCa, size_t rootCaLen, const u8_t *cert,
		    size_t certLen, const u8_t *key, size_t keyLen)
{
	return tls_init(rootCa, rootCaLen, cert, certLen, key, keyLen);
}

int laird_coap_get_server_addr(void)
{
	int r = 0;

#ifdef CONFIG_DNS_RESOLVER
	int dns_retries;
	/* Resolve server with DNS */
	server_addr.ai_family = AF_INET;
	server_addr.ai_socktype = SOCK_DGRAM;
	dns_retries = DNS_RETRIES;
	do {
		r = getaddrinfo(SERVER_HOST, SERVER_PORT_STR, &server_addr,
				&saddr);
		if (r != 0) {
			k_sleep(K_SECONDS(DNS_RETRY_TIME_SECONDS));
		}
		dns_retries--;
	} while (r != 0 && dns_retries != 0);
	if (r != 0) {
		COAP_LOG_ERR("Unable to resolve '%s'", SERVER_HOST);
	}
#endif /* CONFIG_DNS_RESOLVER */

	return r;
}

int laird_coap_start_client()
{
	int ret = 0;
	struct sockaddr_in dest;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_DTLS_1_2);
	if (sock < 0) {
		COAP_LOG_ERR("creating socket failed");
		return 1;
	}

	ret = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sock_sec_tags,
			 sizeof(sock_sec_tags));
	if (ret < 0) {
		COAP_LOG_ERR("Socket options (%d)", ret);
		return ret;
	}

	ret = setsockopt(sock, SOL_TLS, TLS_HOSTNAME, SERVER_HOST,
			 sizeof(SERVER_HOST));
	if (ret < 0) {
		COAP_LOG_ERR("Socket host name (%d)", ret);
		return ret;
	}

#ifdef CONFIG_DNS_RESOLVER
	dest.sin_family = saddr->ai_family;
	dest.sin_port = htons(strtol(SERVER_PORT_STR, NULL, 0));
	net_ipaddr_copy(&dest.sin_addr, &net_sin(saddr->ai_addr)->sin_addr);
#else
	// For use with sever IP addr instead of DNS
	dest.sin_family = AF_INET;
	dest.sin_port = htons(strtol(SERVER_PORT_STR, NULL, 0));
	ret = net_addr_pton(AF_INET, SERVER_IP_ADDR, &dest.sin_addr);
	if (ret < 0) {
		COAP_LOG_ERR("setting dest IP addr failed (%d)", ret);
		return ret;
	}
#endif /* CONFIG_DNS_RESOLVER */

	ret = connect(sock, (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		COAP_LOG_ERR("connect socket failed (%d)", ret);
		return ret;
	}

	prepare_fds(sock);

	return 0;
}

int laird_coap_send_request(enum coap_method method, const char *path,
			    u8_t *payload, u16_t payloadSize)
{
	struct coap_packet request;
	u8_t *data;
	int r;

	data = (u8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN, 1, COAP_TYPE_CON,
			     8, coap_next_token(), method, coap_next_id());
	if (r < 0) {
		COAP_LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, path,
				      strlen(path));
	if (r < 0) {
		COAP_LOG_ERR("Unable add option to request");
		goto end;
	}

	switch (method) {
	case COAP_METHOD_GET:
	case COAP_METHOD_DELETE:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			COAP_LOG_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, payload, payloadSize);
		if (r < 0) {
			COAP_LOG_ERR("Not able to append payload");
			goto end;
		}

		break;
	default:
		r = -EINVAL;
		goto end;
	}

	LOG_HEXDUMP_INF(request.data, request.offset, "Request");

	r = send(sock, request.data, request.offset, 0);
	if (r < 0) {
		COAP_LOG_ERR("Send req err [%d]", r);
	}

end:
	k_free(data);

	return r;
}

static int process_coap_reply(struct coap_packet *reply,
			      const u8_t **replyPayload, u16_t *replyPayloadLen)
{
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
		goto err;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto err;
	}

	LOG_HEXDUMP_INF(data, rcvd, "Response");

	ret = coap_packet_parse(reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		COAP_LOG_ERR("Invalid data received");
		goto err;
	}

	*replyPayload = coap_packet_get_payload(reply, replyPayloadLen);

	goto done;
err:
	k_free(data);
done:
	return ret;
}

int laird_coap_send_request_wait_for_reply(enum coap_method method,
					   const char *path, u8_t *payload,
					   u16_t payloadSize,
					   struct coap_packet *reply,
					   const u8_t **replyPayload,
					   u16_t *replyPayloadLen)
{
	int r;

	r = laird_coap_send_request(method, path, payload, payloadSize);
	if (r < 0) {
		return r;
	}

	r = process_coap_reply(reply, replyPayload, replyPayloadLen);
	if (r < 0) {
		return r;
	}
	return 0;
}

void laird_coap_free_pkt_data(struct coap_packet *reply)
{
	k_free(reply->data);
}

int laird_coap_close_client()
{
	clear_fds();
	return close(sock);
}