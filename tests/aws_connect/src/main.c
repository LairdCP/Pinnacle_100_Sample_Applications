/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(aws_connect, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <net/socket.h>
#include <net/mqtt.h>

#include <string.h>
#include <errno.h>

#include "config.h"

/* Buffers for MQTT client. */
static u8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static u8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

static struct pollfd fds[1];
static int nfds;

static bool connected;

static struct net_mgmt_event_callback mgmt_cb;
struct k_sem iface_ready;
static struct addrinfo server_addr;
struct addrinfo *saddr;
static int connectFailures = 0;
static int connectSuccesses = 0;

#if defined(CONFIG_MQTT_LIB_TLS)

#include "certificate.h"

#define APP_CA_CERT_TAG CA_TAG
#define APP_DEVICE_CERT_TAG DEVICE_CERT_TAG

static sec_tag_t m_sec_tags[] = {
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	APP_CA_CERT_TAG, APP_DEVICE_CERT_TAG
#endif
};

static int tls_init(void)
{
	int err = -EINVAL;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	err = tls_credential_add(APP_DEVICE_CERT_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 dev_certificate, sizeof(dev_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register device certificate: %d", err);
		return err;
	}

	err = tls_credential_add(APP_DEVICE_CERT_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY, dev_key,
				 sizeof(dev_key));
	if (err < 0) {
		LOG_ERR("Failed to register device key: %d", err);
		return err;
	}
#endif

	return err;
}

#endif /* CONFIG_MQTT_LIB_TLS */

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}
#if defined(CONFIG_MQTT_LIB_TLS)
	else if (client->transport.type == MQTT_TRANSPORT_SECURE) {
		fds[0].fd = client->transport.tls.sock;
	}
#endif

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static void wait(int timeout)
{
	if (nfds > 0) {
		if (poll(fds, nfds, timeout) < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		connected = true;
		LOG_INF("[%s:%d] MQTT client connected!", __func__, __LINE__);

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("[%s:%d] MQTT client disconnected %d", __func__,
			__LINE__, evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		LOG_INF("[%s:%d] PUBACK packet id: %u", __func__, __LINE__,
			evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error %d", evt->result);
			break;
		}

		LOG_INF("[%s:%d] PUBREC packet id: %u", __func__, __LINE__,
			evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_ERR("Failed to send MQTT PUBREL: %d", err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		LOG_INF("[%s:%d] PUBCOMP packet id: %u", __func__, __LINE__,
			evt->param.pubcomp.message_id);

		break;

	default:
		break;
	}
}

static char *get_mqtt_payload(enum mqtt_qos qos)
{
	static char payload[30];

	snprintk(payload, sizeof(payload), "{\"data\":{\"temperature\":%d}}",
		 (u8_t)sys_rand32_get());

	return payload;
}

static char *get_mqtt_topic(void)
{
	return "pinnacle100/data";
}

static int publish(struct mqtt_client *client, enum mqtt_qos qos)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (u8_t *)get_mqtt_topic();
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = get_mqtt_payload(qos);
	param.message.payload.len = strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc)                                                 \
	LOG_INF("[%s:%d] %s: %d <%s>", __func__, __LINE__, (func), rc,         \
		RC_STR(rc))

static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = saddr->ai_family;
	broker4->sin_port = htons(strtol(SERVER_PORT_STR, NULL, 0));
	net_ipaddr_copy(&broker4->sin_addr, &net_sin(saddr->ai_addr)->sin_addr);
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (u8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
	client->transport.type = MQTT_TRANSPORT_SECURE;

	struct mqtt_sec_config *tls_config = &client->transport.tls.config;

	tls_config->peer_verify = 2;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
	tls_config->hostname = SERVER_HOST;
#else
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif
}

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {
		client_init(client);

		rc = mqtt_connect(client);
		if (rc != 0) {
			connectFailures++;
			PRINT_RESULT("mqtt_connect", rc);
			k_sleep(APP_SLEEP_MSECS);
			continue;
		}

		prepare_fds(client);

		wait(APP_SLEEP_MSECS);
		mqtt_input(client);

		if (!connected) {
			connectFailures++;
			mqtt_abort(client);
		}
	}

	if (connected) {
		connectSuccesses++;
		return 0;
	}

	return -EINVAL;
}

static int process_mqtt_and_sleep(struct mqtt_client *client, int timeout)
{
	s64_t remaining = timeout;
	s64_t start_time = k_uptime_get();
	int rc;

	while (remaining > 0 && connected) {
		wait(remaining);

		rc = mqtt_live(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_live", rc);
			return rc;
		}

		rc = mqtt_input(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_input", rc);
			return rc;
		}

		remaining = timeout + start_time - k_uptime_get();
	}

	return 0;
}

#define SUCCESS_OR_EXIT(rc)                                                    \
	{                                                                      \
		if (rc != 0) {                                                 \
			return;                                                \
		}                                                              \
	}
#define SUCCESS_OR_BREAK(rc)                                                   \
	{                                                                      \
		if (rc != 0) {                                                 \
			break;                                                 \
		}                                                              \
	}

static void publisher(void)
{
	int i, rc;

	LOG_INF("\r\n\r\nattempting to connect...");
	rc = try_to_connect(&client_ctx);
	PRINT_RESULT("try_to_connect", rc);
	SUCCESS_OR_EXIT(rc);

	i = 0;
	while (i++ < APP_PUBLISH_TRIES && connected) {
		rc = mqtt_ping(&client_ctx);
		PRINT_RESULT("mqtt_ping", rc);
		SUCCESS_OR_BREAK(rc);

		rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
		SUCCESS_OR_BREAK(rc);

		rc = publish(&client_ctx, MQTT_QOS_0_AT_MOST_ONCE);
		PRINT_RESULT("mqtt_publish", rc);
		SUCCESS_OR_BREAK(rc);

		rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
		SUCCESS_OR_BREAK(rc);
	}

	rc = mqtt_disconnect(&client_ctx);
	PRINT_RESULT("mqtt_disconnect", rc);

	wait(APP_SLEEP_MSECS);
	rc = mqtt_input(&client_ctx);
	PRINT_RESULT("mqtt_input", rc);

	LOG_INF("Bye!\r\n");
}

static void iface_evt_handler(struct net_mgmt_event_callback *cb,
			      u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	LOG_INF("iface IPv4 addr added!");
	k_sem_give(&iface_ready);
}

void main(void)
{
	int ret, dns_retries = 0;
	struct net_if *iface;
	struct net_if_config *cfg;

#if defined(CONFIG_MQTT_LIB_TLS)
	ret = tls_init();
	PRINT_RESULT("tls_init", ret);
#endif

	k_sem_init(&iface_ready, 0, 1);

	net_mgmt_init_event_callback(&mgmt_cb, iface_evt_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	/* wait for network interface to be ready */
	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Could not get iface");
		return;
	}

	cfg = net_if_get_config(iface);
	if (!cfg) {
		LOG_ERR("Could not get iface config");
		return;
	}

	/* check if the iface has an IP */
	if (!cfg->ip.ipv4) {
		/* if no IP yet, wait for one */
		LOG_INF("Waiting for network IP addr...");
		k_sem_take(&iface_ready, K_FOREVER);
	}

	server_addr.ai_family = AF_INET;
	server_addr.ai_socktype = SOCK_STREAM;
	dns_retries = DNS_RETRIES;
	do {
		ret = getaddrinfo(SERVER_HOST, SERVER_PORT_STR, &server_addr,
				  &saddr);
		if (ret != 0) {
			k_sleep(K_SECONDS(2));
		}
		dns_retries--;
	} while (ret != 0 && dns_retries != 0);
	if (ret != 0) {
		LOG_ERR("Unable to resolve '%s', quitting", SERVER_HOST);
		return;
	}

	while (true) {
		publisher();
		LOG_INF("\r\nConnection Successes: %d\r\nFailures: %d",
			connectSuccesses, connectFailures);
		k_sleep(K_SECONDS(APP_PUBLISH_INTERVAL_SECS));
	}
}
