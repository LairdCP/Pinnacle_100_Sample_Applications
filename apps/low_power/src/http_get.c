/*
 * Copyright (c) 2020-2023 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_get.h"

#include <stdio.h>

#include <errno.h>
#include <fcntl.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

/* HTTP server to connect to */
#define HTTP_HOST "postman-echo.com"
/* Port to connect to, as string */
#define HTTP_PORT "80"
/* HTTP path to request */
#define HTTP_PATH "/"

#define SOCKET_READ_TRIES	     3
#define SOCKET_READ_RETRY_DELAY_MSEC 1000

#define CHECK(r)                                                                                   \
	{                                                                                          \
		if (r < 0) {                                                                       \
			printf("Error: " #r "\n");                                                 \
			return -1;                                                                 \
		}                                                                                  \
	}

#define CHECK_AND_CLOSE(r)                                                                         \
	{                                                                                          \
		if (r < 0) {                                                                       \
			printf("Error: " #r "\n");                                                 \
			goto close;                                                                \
		}                                                                                  \
	}

#define REQUEST_AND_CLOSE                                                                          \
	"GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST "\r\nConnection: Close\r\n\r\n"

#define REQUEST_KEEP_ALIVE                                                                         \
	"GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST "\r\nConnection: Keep-Alive\r\n\r\n"

#define RESPONSE_SIZE 2048

static char response[RESPONSE_SIZE];
static int sock = -1;

static int set_socket_blocking(int fd, bool val)
{
	int fl, res;

	fl = fcntl(fd, F_GETFL, 0);
	if (fl == -1) {
		return errno;
	}

	if (val) {
		fl &= ~O_NONBLOCK;
	} else {
		fl |= O_NONBLOCK;
	}

	res = fcntl(fd, F_SETFL, fl);
	if (fl == -1) {
		return errno;
	}

	return 0;
}

int http_get_execute(bool keep_alive)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, len, rx_tries;
	char *request;

	if (sock == -1) {
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		printf("Resolving host address %s ...\n", HTTP_HOST);
		st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
		if (st != 0) {
			printf("Unable to resolve address [%d], quitting\n", st);
			return st;
		} else {
			printf("Got host address!\n");
		}

		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		CHECK(sock);

		set_socket_blocking(sock, false);

		printf("Connecting to " HTTP_HOST "\n");

		CHECK_AND_CLOSE(connect(sock, res->ai_addr, res->ai_addrlen));
	}

	if (keep_alive) {
		request = REQUEST_KEEP_ALIVE;
		printf("Keep HTTP connection alive\n");
	} else {
		request = REQUEST_AND_CLOSE;
		printf("Close HTTP connection after request\n");
	}

	printf("\nSending GET request...\n");
	CHECK_AND_CLOSE(send(sock, request, strlen(request), 0));

	printf("Response:\n\n");
	rx_tries = 0;
	while (rx_tries < SOCKET_READ_TRIES) {
		rx_tries++;
		len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			k_sleep(K_MSEC(SOCKET_READ_RETRY_DELAY_MSEC));
			continue;
		} else if (len == 0) {
			goto close;
		}
		rx_tries = 0;
		response[len] = 0;
		printf("%s", response);
	}
	printf("\nResponse received\n");
	if (keep_alive) {
		goto done;
	}

close:
	printf("\nClose connection.\n");
	(void)close(sock);
	sock = -1;
done:
	return 0;
}
