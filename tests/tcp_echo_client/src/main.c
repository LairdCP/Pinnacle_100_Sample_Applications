
#include <zephyr.h>
#include <misc/printk.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(tcp_echo_c);

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/socket.h>

#define SERVER_IP_ADDR "34.211.227.211"
#define SERVER_PORT 12345
#define SEND_MESSAGE "Hello, this is a TCP test message"
#define CLOSE_MSG "close"

static char response[sizeof(SEND_MESSAGE)];

static struct net_mgmt_event_callback mgmt_cb;
struct k_sem iface_ready;

static void iface_evt_handler(struct net_mgmt_event_callback *cb,
			      u32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	LOG_INF("iface IPv4 addr added!");
	k_sem_give(&iface_ready);
}

static int test_tcp_connect_send()
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

	ret = send(sock, SEND_MESSAGE, strlen(SEND_MESSAGE), 0);
	if (ret < 1) {
		LOG_ERR("sending data failed (%d)", ret);
		return ret;
	}

	LOG_INF("Receiving data...");
	len = recv(sock, response, sizeof(response) - 1, 0);
	if (len < 0) {
		LOG_ERR("Error RX data (%d)", len);
		return len;
	}

	if (len == 0) {
		LOG_ERR("RX 0 bytes");
		return -1;
	}

	response[len] = 0;
	LOG_INF("RX: %s", response);

	ret = close(sock);
	if (ret < 0) {
		LOG_ERR("closing socket failed");
		return ret;
	}

	return ret;
}

static int test_tcp_connect_send_server_close()
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

	ret = send(sock, SEND_MESSAGE, strlen(SEND_MESSAGE), 0);
	if (ret < 1) {
		LOG_ERR("sending data failed (%d)", ret);
		goto close;
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
			goto done;
		}

		response[len] = 0;
		LOG_INF("RX: %s", response);

		/* now send close so server closes connection */
		ret = send(sock, CLOSE_MSG, strlen(CLOSE_MSG), 0);
		if (ret < 1) {
			LOG_ERR("sending data failed (%d)", ret);
			goto close;
		}
	}

close:
	ret = close(sock);
	if (ret < 0) {
		LOG_ERR("closing socket failed");
		goto done;
	}
done:
	return ret;
}

void main(void)
{
	int ret;
	struct net_if *iface;
	struct net_if_config *cfg;

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

	LOG_INF("Starting tests...");
	/* run tests */
	LOG_INF("Run test_tcp_connect_send");
	ret = test_tcp_connect_send();
	if (ret < 0) {
		LOG_ERR("test_tcp_connect_send failed");
		return;
	}

	LOG_INF("Run test_tcp_connect_send_server_close");
	ret = test_tcp_connect_send_server_close();
	if (ret < 0) {
		LOG_ERR("test_tcp_connect_send_server_close failed");
		return;
	}

	LOG_INF("Done");
}