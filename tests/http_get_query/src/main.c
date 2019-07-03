
#include <zephyr.h>
#include <misc/printk.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(http_get_query);

#include <stdio.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/socket.h>

#define HTTP_HOST "uwterminalx.no-ip.org"
#define HTTP_PORT_STR "80"
/* HTTP path to request */
#define HTTP_PATH "/pinnacle_tests/"
#define POST_FILE "post.php"
#define QUERY_PARM "In"

#define REQUEST                                                                \
	"GET " HTTP_PATH POST_FILE "?" QUERY_PARM "=%s HTTP/1.1\r\nHost: " HTTP_HOST                       \
	"\r\nAccept: text\r\nConnection: Close\r\n\r\n"

#define DNS_RETRIES 3

static char senddata[1501];
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

static int test_http_get_query()
{
	int ret, len, sock, dns_retries = 0;
	static struct addrinfo hints;
	struct addrinfo *res;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	dns_retries = DNS_RETRIES;
	do {
		ret = getaddrinfo(HTTP_HOST, HTTP_PORT_STR, &hints, &res);
		if (ret != 0) {
			k_sleep(K_SECONDS(2));
		}
		dns_retries--;
	} while (ret != 0 && dns_retries != 0);
	if (ret != 0) {
		printk("Unable to resolve address, quitting\n");
		goto done;
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock < 0) {
		LOG_ERR("creating socket failed");
		ret = 1;
		goto done;
	}

	ret = connect(sock, res->ai_addr, res->ai_addrlen);
	if (ret < 0) {
		LOG_ERR("connect socket failed (%d)", ret);
		goto done;
	}

	sprintf(senddata, REQUEST, "Lorem%20ipsum%20dolor%20sit%20amet,%20consectetur%20adipiscing%20elit,"
	"sed%20do%20eiusmod%20tempor%20incididunt%20ut%20labore%20et%20dolore%20magna%20aliqua.%20Urna%20id"
	"volutpat%20lacus%20laoreet%20non.%20Elit%20pellentesque%20habitant%20morbi%20tristique"
	"senectus%20et.%20Penatibus%20et%20magnis%20dis%20parturient%20montes%20nascetur%20ridiculus"
	"mus%20mauris.%20Arcu%20non%20sodales%20neque%20sodales%20ut%20etiam%20sit.%20Amet%20venenatis"
	"urna%20cursus%20eget%20nunc.%20In%20mollis%20nunc%20sed%20id%20semper%20risus%20in%20hendrerit."
	"Tristique%20senectus%20et%20netus%20et.%20Mus%20mauris%20vitae%20ultricies%20leo%20integer"
	"malesuada%20nunc.%20Ut%20ornare%20lectus%20sit%20amet%20est%20placerat%20in%20egestas%20erat."
	"Blandit%20turpis%20cursus%20in%20hac%20habitasse%20platea%20dictumst.%20Dui%20nunc%20mattis");

	ret = send(sock, senddata, strlen(senddata), 0);
	if (ret < 1) {
		LOG_ERR("sending data failed (%d)", ret);
		goto close;
	}

	printk("\r\n\r\nResponse:\r\n\r\n");
	while (1) {
		len = recv(sock, response, sizeof(response) - 1, 0);

		if (len == -EAGAIN) {
			LOG_ERR("RX timeout (%d)", len);
			goto close;
		} else if (len == -EWOULDBLOCK) {
			LOG_ERR("RX EWOULDBLOCK (%d)", len);
			goto close;
		} else if (len < 0) {
			LOG_ERR("RX unknown err (%d)", len);
			goto close;
		} else if (len == 0) {
			/* socket shutdown properly */
			LOG_INF("Server closed socket");
			goto close;
		}

		response[len] = 0;
		printk("%s", response);
	}

	printk("\r\n");

close:
	LOG_INF("Closing socket");
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

	int run = 1;
	while (run < 12)
	{
		LOG_INF("Run test_http_get_query #%d", run);
		ret = test_http_get_query();
		if (ret < 0) {
			LOG_ERR("test_http_get_query #%d failed", run);
			return;
		}
		++run;
	}

	LOG_INF("Done");
}