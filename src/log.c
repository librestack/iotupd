/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2019-2022 Brett Sheffield <brett@librecast.net> */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <lwmon/packet.h>
#include "log.h"
#include "misc.h"

#define LWMON_PATH "/tmp/lwmon.socket"

void logmsg(unsigned int level, const char *fmt, ...)
{
	va_list argp;
	char *b;
	if (isatty(STDERR_FILENO)) return;
	va_start(argp, fmt);
	b = malloc(_vscprintf(fmt, argp) + 1);
	assert(b != NULL);
	vsprintf(b, fmt, argp);
	va_end(argp);
	fprintf(stderr, "%s\n", b);
	free(b);
}

void lwmon_log(const char * who, const char * what)
{
	static int lwmon_fd = -1;
	lwmon_packet_t P;
	if (lwmon_fd == -1) {
		struct sockaddr_un ls = {
			.sun_family = AF_UNIX,
			.sun_path   = LWMON_PATH,
		};
		int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (fd == -1) return;
		if (connect(fd, (struct sockaddr *)&ls, sizeof(ls)) == -1) {
			close(fd);
			return;
		}
		lwmon_fd = fd;
	}
	lwmon_initpacket(&P);
	lwmon_add_log(&P, who, what);
	lwmon_add_command(&P, lwmon_command_measure);
	lwmon_closepacket(&P);
	lwmon_sendpacket(lwmon_fd, &P);
}
