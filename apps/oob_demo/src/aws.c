/*
 * Copyright (c) 2019 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(oob_aws);

#include <net/socket.h>
#include <net/mqtt.h>

#include "certificate.h"
#include "aws.h"

#ifndef CONFIG_MQTT_LIB_TLS
#error "AWS must use MQTT with TLS"
#endif

#define APP_CA_CERT_TAG CA_TAG
#define APP_DEVICE_CERT_TAG DEVICE_CERT_TAG

#define AWS_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#define AWS_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define AWS_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define AWS_LOG_DBG(...) LOG_DBG(__VA_ARGS__)

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

static struct addrinfo server_addr;
struct addrinfo *saddr;

static sec_tag_t m_sec_tags[] = { APP_CA_CERT_TAG, APP_DEVICE_CERT_TAG };

static struct shadow_reported_struct shadow_persistent_data;

static int tls_init(void)
{
	int err = -EINVAL;

	err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate, sizeof(ca_certificate));
	if (err < 0) {
		AWS_LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	err = tls_credential_add(APP_DEVICE_CERT_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 dev_certificate, sizeof(dev_certificate));
	if (err < 0) {
		AWS_LOG_ERR("Failed to register device certificate: %d", err);
		return err;
	}

	err = tls_credential_add(APP_DEVICE_CERT_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY, dev_key,
				 sizeof(dev_key));
	if (err < 0) {
		AWS_LOG_ERR("Failed to register device key: %d", err);
		return err;
	}

	return err;
}

static void prepare_fds(struct mqtt_client *client)
{
	fds[0].fd = client->transport.tls.sock;
	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	fds[0].fd = 0;
	nfds = 0;
}

static void wait(int timeout)
{
	if (nfds > 0) {
		if (poll(fds, nfds, timeout) < 0) {
			AWS_LOG_ERR("poll error: %d", errno);
		}
	}
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			AWS_LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		connected = true;
		AWS_LOG_INF("[%s:%d] MQTT client connected!", __func__,
			    __LINE__);

		break;

	case MQTT_EVT_DISCONNECT:
		AWS_LOG_INF("[%s:%d] MQTT client disconnected %d", __func__,
			    __LINE__, evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			AWS_LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		AWS_LOG_INF("[%s:%d] PUBACK packet id: %u", __func__, __LINE__,
			    evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBLISH:
		if (evt->result != 0) {
			AWS_LOG_ERR("MQTT PUBLISH error %d", evt->result);
			break;
		}

		AWS_LOG_INF("MQTT RXd ID: %d Topic: %s len: %d",
			    evt->param.publish.message_id,
			    evt->param.publish.message.topic.topic.utf8,
			    evt->param.publish.message.payload.len);

		break;
	default:
		break;
	}
}

static char *get_mqtt_topic(void)
{
	static char topic[sizeof(
		"$aws/things/deviceId-###############/shadow/update")];

	snprintk(topic, sizeof(topic), "$aws/things/deviceId-%s/shadow/update",
		 shadow_persistent_data.state.reported.IMEI);

	return topic;
}

static int publish_string(struct mqtt_client *client, enum mqtt_qos qos,
			  char *data)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (u8_t *)get_mqtt_topic();
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = data;
	param.message.payload.len = strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

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

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
	client->transport.type = MQTT_TRANSPORT_SECURE;
	struct mqtt_sec_config *tls_config = &client->transport.tls.config;
	tls_config->peer_verify = 2;
	tls_config->cipher_list = NULL;
	tls_config->sec_tag_list = m_sec_tags;
	tls_config->sec_tag_count = ARRAY_SIZE(m_sec_tags);
	tls_config->hostname = SERVER_HOST;
}

/* In this routine we block until the connected variable is 1 */
static int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {
		client_init(client);

		rc = mqtt_connect(client);
		if (rc != 0) {
			AWS_LOG_ERR("mqtt_connect (%d)", rc);
			k_sleep(APP_SLEEP_MSECS);
			continue;
		}

		prepare_fds(client);

		wait(APP_SLEEP_MSECS);
		mqtt_input(client);

		if (!connected) {
			mqtt_abort(client);
		}
	}

	if (connected) {
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

		// rc = mqtt_live(client);
		// if (rc != 0) {
		// 	AWS_LOG_ERR("mqtt_live (%d)", rc);
		// 	return rc;
		// }

		rc = mqtt_input(client);
		if (rc != 0) {
			AWS_LOG_ERR("mqtt_input (%d)", rc);
			return rc;
		}

		remaining = timeout + start_time - k_uptime_get();
	}

	return 0;
}

int awsInit(void)
{
	int rc;
	rc = tls_init();

	/* init shadow data */
	shadow_persistent_data.state.reported.firmware_version = "";
	shadow_persistent_data.state.reported.os_version = "";
	shadow_persistent_data.state.reported.radio_version = "";
	shadow_persistent_data.state.reported.IMEI = "";

	return rc;
}

int awsGetServerAddr(void)
{
	int rc, dns_retries;

	server_addr.ai_family = AF_INET;
	server_addr.ai_socktype = SOCK_STREAM;
	dns_retries = DNS_RETRIES;
	do {
		LOG_DBG("Get AWS server address");
		rc = getaddrinfo(SERVER_HOST, SERVER_PORT_STR, &server_addr,
				 &saddr);
		if (rc != 0) {
			k_sleep(K_SECONDS(5));
		}
		dns_retries--;
	} while (rc != 0 && dns_retries != 0);
	if (rc != 0) {
		AWS_LOG_ERR("Unable to resolve '%s'", SERVER_HOST);
	}

	return rc;
}

int awsConnect(void)
{
	AWS_LOG_INF("Attempting to connect to AWS...");
	int rc = try_to_connect(&client_ctx);
	if (rc != 0) {
		AWS_LOG_ERR("AWS connect err (%d)", rc);
	}
	return rc;
}

void awsDisconnect(void)
{
	mqtt_disconnect(&client_ctx);
	process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
}

int awsKeepAlive(void)
{
	int rc;

	rc = mqtt_live(&client_ctx);
	if (rc != 0) {
		AWS_LOG_ERR("mqtt_live (%d)", rc);
		goto done;
	}

	rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
	if (rc != 0) {
		goto done;
	}
done:
	return rc;
}

int awsSetShadowKernelVersion(const char *version)
{
	shadow_persistent_data.state.reported.os_version = version;

	return 0;
}

int awsSetShadowIMEI(const char *imei)
{
	shadow_persistent_data.state.reported.IMEI = imei;

	return 0;
}

int awsSetShadowRadioFirmwareVersion(const char *version)
{
	shadow_persistent_data.state.reported.radio_version = version;

	return 0;
}

int awsSetShadowAppFirmwareVersion(const char *version)
{
	shadow_persistent_data.state.reported.firmware_version = version;

	return 0;
}

static int sendData(char *data)
{
	int rc;

	rc = publish_string(&client_ctx, MQTT_QOS_0_AT_MOST_ONCE, data);
	if (rc != 0) {
		AWS_LOG_ERR("MQTT publish err (%d)", rc);
		goto done;
	}

	rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
	if (rc != 0) {
		AWS_LOG_ERR("process MQTT err (%d)", rc);
		goto done;
	}
done:
	return rc;
}

int awsSendData(char *data)
{
	return sendData(data);
}

int awsPublishShadowPersistentData()
{
	int rc;
	size_t buf_len;

	buf_len = json_calc_encoded_len(shadow_descr, ARRAY_SIZE(shadow_descr),
					&shadow_persistent_data);
	char msg[buf_len];

	rc = json_obj_encode_buf(shadow_descr, ARRAY_SIZE(shadow_descr),
				 &shadow_persistent_data, msg, sizeof(msg));
	if (rc < 0) {
		goto done;
	}

	rc = sendData(msg);
done:
	return rc;
}