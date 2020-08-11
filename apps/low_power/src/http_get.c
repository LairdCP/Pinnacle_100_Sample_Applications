/*
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <net/socket.h>
#include <kernel.h>

#include "http_get.h"

/* HTTP server to connect to */
#define HTTP_HOST "postman-echo.com"
/* Port to connect to, as string */
#define HTTP_PORT "80"
/* HTTP path to request */
#define HTTP_PATH "/"

#define SSTRLEN(s) (sizeof(s) - 1)
#define CHECK(r)                                                               \
	{                                                                      \
		if (r < 0) {                                                   \
			printf("Error: " #r "\n");                             \
			return -1;                                             \
		}                                                              \
	}

#define CHECK_AND_CLOSE(r)                                                     \
	{                                                                      \
		if (r < 0) {                                                   \
			printf("Error: " #r "\n");                             \
			goto close;                                            \
		}                                                              \
	}

#define REQUEST                                                                \
	"GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST                       \
	"\r\nConnection: Close\r\n\r\n"

/* uncomment to test leaving the connection open as long as possible */
/* #define REQUEST                                                                \
	"GET " HTTP_PATH " HTTP/1.1\r\nHost: " HTTP_HOST                       \
	"\r\nConnection: Keep-Alive\r\n\r\n"
 */

#define RESPONSE_SIZE 1024

static char response[RESPONSE_SIZE];

int http_get_execute(void)
{
	static struct addrinfo hints;
	struct addrinfo *res;
	int st, sock;

	printf("Preparing HTTP GET request for http://" HTTP_HOST
	       ":" HTTP_PORT HTTP_PATH "\n");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	printf("Resolving host address...\n");
	st = getaddrinfo(HTTP_HOST, HTTP_PORT, &hints, &res);
	if (st != 0) {
		printf("Unable to resolve address [%d], quitting\n", st);
		return st;
	} else {
		printf("Got host address!\n");
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	CHECK(sock);

	printf("Sending GET request...\n");
	CHECK_AND_CLOSE(connect(sock, res->ai_addr, res->ai_addrlen));
	CHECK_AND_CLOSE(send(sock, REQUEST, SSTRLEN(REQUEST), 0));

	printf("Response:\n\n");
	while (1) {
		int len = recv(sock, response, sizeof(response) - 1, 0);

		if (len < 0) {
			printf("Error reading response\n");
			goto close;
		}

		if (len == 0) {
			break;
		}

		response[len] = 0;
		printf("%s", response);
	}

close:
	printf("\nClose connection.\n");
	(void)close(sock);

	return 0;
}