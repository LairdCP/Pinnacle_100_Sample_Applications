
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
#define GET_BIG_DATA_MSG "getBigData"
#define MSG1_LOOP_TIMES 10

static const char LARGE_MSG[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit,\n"
	"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna id\n"
	"volutpat lacus laoreet non. Elit pellentesque habitant morbi tristique\n"
	"senectus et. Penatibus et magnis dis parturient montes nascetur ridiculus\n"
	"mus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatis\n"
	"urna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.\n"
	"Tristique senectus et netus et. Mus mauris vitae ultricies leo integer\n"
	"malesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.\n"
	"Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis\n"
	"enim ut tellus elementum sagittis. Molestie nunc non blandit massa enim\n"
	"nec. Sed viverra tellus in hac habitasse platea dictumst vestibulum.\n"
	"Eget est lorem ipsum dolor sit amet.\n"
	"Auctor augue mauris augue neque gravida in fermentum. Nulla pellentesque\n"
	"dignissim enim sit amet. Pharetra magna ac placerat vestibulum lectus mauris\n"
	"ultrices eros in. Volutpat lacus laoreet non curabitur. Neque vitae tempus\n"
	"quam pellentesque nec nam. Semper eget duis at tellus at urna condimentum\n"
	"mattis pellentesque. Viverra maecenas accumsan lacus vel facilisis volutpat\n"
	"est. Tristique risus nec feugiat in fermentum posuere urna nec. Amet commodo\n"
	"nulla facilisi nullam vehicula ipsum a arcu cursus. Gravida dictum fusce ut\n"
	"placerat orci nulla pellentesque. Diam vulputate ut pharetra sit amet aliquam\n"
	"id diam maecenas. Aliquam vestibulum morbi blandit cursus risus. Blandit\n"
	"turpis cursus in hac habitasse platea dictumst quisque sagittis.\n"
	"Adipiscing bibendum est ultricies integer quis.\n"
	"Sed faucibus turpis in eu mi. Viverra maecenas accumsan lacus vel facilisis\n"
	"volutpat est velit egestas. Egestas tellus rutrum tellus pellentesque eu.\n"
	"Et malesuada fames ac turpis egestas integer. Elit sed vulputate mi sit\n"
	"amet mauris commodo quis imperdiet. Mus mauris vitae ultricies leo integer\n"
	"malesuada nunc vel. Leo vel orci porta non pulvinar. Laoreet non curabitur\n"
	"gravida arcu ac tortor dignissim convallis aenean. Dictum varius duis at\n"
	"consectetur lorem. Tempus imperdiet nulla malesuada pellentesque. Tellus\n"
	"rutrum tellus pellentesque eu tincidunt tortor. Vel elit scelerisque mauris\n"
	"pellentesque pulvinar pellentesque habitant morbi. Sollicitudin nibh sit amet\n"
	"commodo nulla facilisi nullam vehicula. Vel pretium lectus quam id leo in\n"
	"vitae. Dolor sit amet consectetur adipiscing elit ut aliquam purus sit.\n"
	"Cras semper auctor neque vitae tempus quam pellentesque. Aliquam ultrices\n"
	"sagittis orci a scelerisque. Amet porttitor eget dolor morbi.\n\n"
	"   Lorem ipsum dolor sit amet, consectetur adipiscing elit,\n"
	"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna id\n"
	"volutpat lacus laoreet non. Elit pellentesque habitant morbi tristique\n"
	"senectus et. Penatibus et magnis dis parturient montes nascetur ridiculus\n"
	"mus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatis\n"
	"urna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.\n"
	"Tristique senectus et netus et. Mus mauris vitae ultricies leo integer\n"
	"malesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.\n"
	"Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis\n"
	"enim ut tellus elementum sagittis. Molestie nunc non blandit massa enim\n"
	"nec. Sed viverra tellus in hac habitasse platea dictumst vestibulum.\n"
	"Eget est lorem ipsum dolor sit amet.\n"
	"Auctor augue mauris augue neque gravida in fermentum. Nulla pellentesque\n"
	"dignissim enim sit amet. Pharetra magna ac placerat vestibulum lectus mauris\n"
	"ultrices eros in. Volutpat lacus laoreet non curabitur. Neque vitae tempus\n"
	"quam pellentesque nec nam. Semper eget duis at tellus at urna condimentum\n"
	"mattis pellentesque. Viverra maecenas accumsan lacus vel facilisis volutpat\n"
	"est. Tristique risus nec feugiat in fermentum posuere urna nec. Amet commodo\n"
	"nulla facilisi nullam vehicula ipsum a arcu cursus. Gravida dictum fusce ut\n"
	"placerat orci nulla pellentesque. Diam vulputate ut pharetra sit amet aliquam\n"
	"id diam maecenas. Aliquam vestibulum morbi blandit cursus risus. Blandit\n"
	"turpis cursus in hac habitasse platea dictumst quisque sagittis.\n"
	"Adipiscing bibendum est ultricies integer quis.\n"
	"Sed faucibus turpis in eu mi. Viverra maecenas accumsan lacus vel facilisis\n"
	"volutpat est velit egestas. Egestas tellus rutrum tellus pellentesque eu.\n"
	"Et malesuada fames ac turpis egestas integer. Elit sed vulputate mi sit\n"
	"amet mauris commodo quis imperdiet. Mus mauris vitae ultricies leo integer\n"
	"malesuada nunc vel. Leo vel orci porta non pulvinar. Laoreet non curabitur\n"
	"gravida arcu ac tortor dignissim convallis aenean. Dictum varius duis at\n"
	"consectetur lorem. Tempus imperdiet nulla malesuada pellentesque. Tellus\n"
	"rutrum tellus pellentesque eu tincidunt tortor. Vel elit scelerisque mauris\n"
	"pellentesque pulvinar pellentesque habitant morbi. Sollicitudin nibh sit amet\n"
	"commodo nulla facilisi nullam vehicula. Vel pretium lectus quam id leo in\n"
	"vitae. Dolor sit amet consectetur adipiscing elit ut aliquam purus sit.\n"
	"Cras semper auctor neque vitae tempus quam pellentesque. Aliquam ultrices\n"
	"sagittis orci a scelerisque. Amet porttitor eget dolor morbi.";

static const char MSG1[] =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit,\n"
	"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Urna id\n"
	"volutpat lacus laoreet non. Elit pellentesque habitant morbi tristique\n"
	"senectus et. Penatibus et magnis dis parturient montes nascetur ridiculus\n"
	"mus mauris. Arcu non sodales neque sodales ut etiam sit. Amet venenatis\n"
	"urna cursus eget nunc. In mollis nunc sed id semper risus in hendrerit.\n"
	"Tristique senectus et netus et. Mus mauris vitae ultricies leo integer\n"
	"malesuada nunc. Ut ornare lectus sit amet est placerat in egestas erat.\n"
	"Blandit turpis cursus in hac habitasse platea dictumst. Dui nunc mattis\n"
	"enim ut tellus elementum sagittis. Molestie nunc non blandit massa enim\n"
	"nec. Sed viverra tellus in hac habitasse platea dictumst vestibulum.\n"
	"Eget est lorem ipsum dolor sit amet.\n"
	"Auctor augue mauris augue neque gravida in fermentum. Nulla pellentesque\n"
	"dignissim enim sit amet. Pharetra magna ac placerat vestibulum lectus mauris\n"
	"ultrices eros in. Volutpat lacus laoreet non curabitur. Neque vitae tempus.";

typedef int TEST(void);

static char response[sizeof(SEND_MESSAGE)];
static char large_response[sizeof(LARGE_MSG)];
static char msg1_response[sizeof(MSG1)];

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
			break;
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
	close(sock);
	return ret;
}

static int test_tcp_send_rx_large_data()
{
	int ret, len, sock, send_size, bytes_sent, rx_size, total_rx, to_send,
		to_rx;
	ret = len = sock = send_size = bytes_sent = rx_size = total_rx = 0;
	struct sockaddr_in dest;
	to_send = to_rx = strlen(LARGE_MSG);

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

	send_size = rx_size = to_send;
	LOG_INF("Send data (len:%d)...", send_size);
	char *msg = (char *)LARGE_MSG;
	do {
		ret = send(sock, msg, send_size, 0);
		if (ret < 1) {
			LOG_ERR("sending data failed (%d)", ret);
			goto close;
		}
		bytes_sent += ret;
		send_size -= bytes_sent;
		msg += bytes_sent;
		LOG_INF("Sent %d bytes, total: %d", ret, bytes_sent);
	} while (bytes_sent < to_send);

	char *rx_msg = large_response;
	while (true) {
		LOG_INF("Receiving data...");
		len = recv(sock, rx_msg, rx_size, 0);

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

		total_rx += len;
		rx_size -= len;
		rx_msg += len;
		LOG_INF("RX %d bytes, total: %d", len, total_rx);
		if (total_rx >= to_rx) {
			break;
		}
	}

	ret = strcmp(LARGE_MSG, large_response);
	if (ret != 0) {
		LOG_ERR("Send and RX data do not match!");
	}

close:
	close(sock);
	return ret;
}

static int test_tcp_get_large_data()
{
	int ret, len, sock, rx_size, total_rx, to_rx;
	ret = len = sock = rx_size = total_rx = 0;
	struct sockaddr_in dest;
	rx_size = to_rx = strlen(LARGE_MSG);

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

	ret = send(sock, GET_BIG_DATA_MSG, strlen(GET_BIG_DATA_MSG), 0);
	if (ret < 1) {
		LOG_ERR("sending data failed (%d)", ret);
		goto close;
	}

	char *rx_msg = large_response;
	LOG_INF("Receiving %d bytes...", rx_size);
	while (true) {
		LOG_INF("Receiving data...");
		len = recv(sock, rx_msg, rx_size, 0);

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

		total_rx += len;
		rx_size -= len;
		rx_msg += len;
		LOG_INF("RX %d bytes, total: %d", len, total_rx);
		if (total_rx >= to_rx) {
			break;
		}
	}

	ret = strcmp(LARGE_MSG, large_response);
	if (ret != 0) {
		LOG_ERR("RX data do not match expected data!");
	}

close:
	close(sock);
	return ret;
}

static int test_tcp_send_rx_msg1_loop()
{
	int ret, len, sock, send_size, bytes_sent, rx_size, total_rx, to_send,
		to_rx, loops;
	ret = len = sock = send_size = bytes_sent = rx_size = total_rx = 0;
	struct sockaddr_in dest;
	to_send = to_rx = strlen(MSG1);
	char *rx_msg;

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

	loops = 0;
	while (loops < MSG1_LOOP_TIMES) {
		loops++;
		LOG_INF("Loop %d", loops);
		bytes_sent = 0, total_rx = 0;
		send_size = rx_size = to_send;
		rx_msg = msg1_response;

		LOG_INF("Sent data (len:%d)...", send_size);
		char *msg = (char *)MSG1;
		do {
			ret = send(sock, msg, send_size, 0);
			if (ret < 1) {
				LOG_ERR("sending data failed (%d)", ret);
				goto close;
			}
			bytes_sent += ret;
			send_size -= bytes_sent;
			msg += bytes_sent;
			LOG_INF("Sent %d bytes, total: %d", ret, bytes_sent);
		} while (bytes_sent < to_send);

		while (true) {
			LOG_INF("Receiving data...");
			len = recv(sock, rx_msg, rx_size, 0);

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

			total_rx += len;
			rx_size -= len;
			rx_msg += len;
			LOG_INF("RX %d bytes, total: %d", len, total_rx);
			if (total_rx >= to_rx) {
				break;
			}
		}
		ret = strcmp(MSG1, msg1_response);
		if (ret != 0) {
			LOG_ERR("Send and RX data do not match!");
		}
	}

close:
	close(sock);
	return ret;
}

TEST *tests[] = { test_tcp_connect_send,
		  test_tcp_connect_send_server_close,
		  test_tcp_send_rx_msg1_loop,
		  test_tcp_get_large_data,
		  test_tcp_send_rx_large_data,
		  NULL };

void main(void)
{
	int ret, testIndex, testNum;
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

	testIndex = 0;

	while (true) {
		TEST *test = tests[testIndex];
		testNum = testIndex + 1;
		if (test) {
			LOG_INF("Running test %d", testNum);
			ret = test();
			if (ret < 0) {
				LOG_ERR("test %d failed (%d)", testNum, ret);
			}
			LOG_INF("Test %d done\r\n", testNum);
			/* Wait some time before starting the next test */
			k_sleep(K_SECONDS(1));
			testIndex++;
		} else {
			break;
		}
	}

	LOG_INF("Done");
}