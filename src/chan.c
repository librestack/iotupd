/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2021 Claudio Calvelli <clc@w42.org> */

#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <librecast.h>
#include "chan.h"

/* get multicast group and port from strings */
int get_channel (const char * group, /* const char * port, */ struct sockaddr_in6 * addr)
{
	memset(addr, 0, sizeof(*addr));
	addr->sin6_family = AF_INET6;
	if (inet_pton(AF_INET6, group, &addr->sin6_addr) < 1) return -1;
	addr->sin6_port = htons(LC_DEFAULT_PORT);
	return 0;
}

