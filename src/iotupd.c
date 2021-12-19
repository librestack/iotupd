/* SPDX-License-Identifier: GPL-2.0-or-later 
 * Copyright (c) 2019-2021 Brett Sheffield <brett@gladserv.com> */

#include "err.h"
#include "iot.h"
#include "log.h"
#include "chan.h"
#include <errno.h>
#include <fcntl.h>
#include <librecast.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>

#define PROGRAM_NAME "iotupd"

static lc_ctx_t *ctx = NULL;
static lc_socket_t *sock = NULL;
static lc_channel_t *chan = NULL;
static int running = 1;
static int fd;
static char *map;
static struct stat sb;

void sigint_handler (int signo);
void terminate();

void sigint_handler (int signo)
{
	running = 0;
}

void terminate()
{
	lc_channel_free(chan);
	lc_socket_close(sock);
	lc_ctx_free(ctx);
	munmap(map, sb.st_size);
	close(fd);
	_exit(0);
}

int main(int argc, char **argv)
{
	struct sockaddr_in6 addr;
	struct iot_frame_t f;
	struct timespec pkt_delay = { 0, };
	const int on = 1;
	int ifindex = 0, pkt_loop = 0;

	if (argc < 3 || argc > 5) {
		fprintf(stderr, "usage: %s <file> <group> [<packet_delay>] [<interface>]\n", argv[0]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		logmsg(LOG_ERROR, "Unable to open file '%s'", argv[1]);
		return IOTD_ERROR_FILE_OPEN_FAIL;
	}

	if (fstat(fd, &sb) == -1) {
		logmsg(LOG_ERROR, "fstat() failed");
		return IOTD_ERROR_FILE_STAT_FAIL;
	}

	logmsg(LOG_DEBUG, "Mapping file '%s' with %lld bytes", argv[1], (long long)sb.st_size);

	map = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		logmsg(LOG_ERROR, "mmap() failed: %s", strerror(errno));
		return IOTD_ERROR_MMAP_FAIL;
	}

	if (get_channel(argv[2], &addr) == -1) {
		logmsg(LOG_ERROR, "Group '%s' not valid", argv[2]);
		return IOTD_ERROR_INVALID_ARGS;
	}

	if (argc > 3) {
		int argn = 3;
		/* is this a packet delay? */
		if (sscanf(argv[argn], "%ld.%d", &pkt_delay.tv_nsec, &pkt_loop) >= 1)
		    argn++;
		if (argc > argn) {
			ifindex = if_nametoindex(argv[argn]);
			if (ifindex == 0) {
				logmsg(LOG_ERROR, "Interface '%s' not found", argv[argn]);
				return IOTD_ERROR_IF_NODEV;
			}
		}
	}

	signal(SIGINT, sigint_handler);

	ctx = lc_ctx_new();
	sock = lc_socket_new(ctx);
	if (lc_socket_bind(sock, ifindex) == -1) {
		logmsg(LOG_ERROR, "Cannot bind to interface '%s'", argv[2]);
		return IOTD_ERROR_IF_NODEV;
	}
	lc_socket_setopt(sock, IPV6_MULTICAST_LOOP, &on, sizeof(on));

	chan = lc_channel_init(ctx, &addr);
	lc_channel_bind(sock, chan);

	memset(&f, 0, sizeof(iot_frame_t));

	/* calculate file hash */
	hash_generic(f.hash, HASHSIZE, (unsigned char *)map, sb.st_size);

	while (running) {
		for (int i = 0; i <= sb.st_size && running; i += MTU_FIXED) {
			f.op = 0; /* TODO: data opcode */
			f.size = sb.st_size;
			f.off = i;

			if ((i + MTU_FIXED) > sb.st_size)
				f.len = sb.st_size - i;
			else
				f.len = MTU_FIXED;

			//logmsg(LOG_DEBUG, "sending %i - %i", i, (int)(i+f.len));

			memcpy(f.data, map + i, f.len);

			lc_channel_send(chan, &f, sizeof(f), 0);
			if (pkt_delay.tv_nsec > 0)
				nanosleep(&pkt_delay, NULL);
			for (int i = 0; i < pkt_loop; i++) ;
		}
	}
	terminate();

	return 0;
}
