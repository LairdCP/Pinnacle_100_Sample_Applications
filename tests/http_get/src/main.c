
#include <zephyr.h>
#include <misc/printk.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(http_get);

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/socket.h>

#define HTTP_HOST_IP "172.217.164.142"
#define HTTP_HOST "www.google.com"
#define HTTP_PORT 80
/* HTTP path to request */
#define HTTP_PATH "/index.html"

#define REQUEST                                                                \
	"GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST                       \
	"\r\nAccept: text\r\nConnection: Close\r\n\r\n"

static char response[1501];

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

static int test_http_get()
{
	int ret, len, sock = 0;
	struct sockaddr_in dest;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		LOG_ERR("creating socket failed");
		return 1;
	}

	// 	/* Put the socket in non-blocking mode */
	// 	if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) < 0) {
	// 		LOG_ERR("Err setting socket to non-blocking");
	// 		return 1;
	// 	}

	dest.sin_family = AF_INET;
	ret = net_addr_pton(AF_INET, HTTP_HOST_IP, &dest.sin_addr);
	if (ret < 0) {
		LOG_ERR("setting dest IP addr failed (%d)", ret);
		return ret;
	}
	dest.sin_port = htons(HTTP_PORT);

	ret = connect(sock, (struct sockaddr *)&dest, sizeof(dest));
	if (ret < 0) {
		LOG_ERR("connect socket failed (%d)", ret);
		return ret;
	}

	ret = send(sock, REQUEST, strlen(REQUEST), 0);
	if (ret < 1) {
		LOG_ERR("sending data failed (%d)", ret);
		return ret;
	}

	printk("\r\n\r\nResponse:\r\n\r\n");
	while (1) {
		len = recv(sock, response, sizeof(response) - 1, 0);

		if (len == -EAGAIN) {
			LOG_ERR("RX timeout (%d)", len);
			return len;
		} else if (len == -EWOULDBLOCK) {
			LOG_ERR("RX EWOULDBLOCK (%d)", len);
			return len;
		} else {
			LOG_ERR("RX unknown err (%d)", len);
			return len;
		}

		if (len == 0) {
			/* socket shutdown properly */
			break;
		}

		response[len] = 0;
		printk("%s", response);
	}

	printk("\r\n");

	ret = close(sock);
	if (ret < 0) {
		LOG_ERR("closing socket failed");
		return ret;
	}

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

	LOG_INF("Run test_http_get");
	ret = test_http_get();
	if (ret < 0) {
		LOG_ERR("test_http_get failed");
		return;
	}

	LOG_INF("Done");
}